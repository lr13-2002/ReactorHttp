#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "HttpRequest.h"
#include "Buffer.h"
#include <assert.h>
#include <ctype.h>
#define _GNU_SOURCE
#define HeaderSize 12
struct HttpRequest* httpRequestInit() {
	struct HttpRequest* request = (struct HttpRequest*)malloc(sizeof(struct HttpRequest));
	
	httpRequestReset(request);
	request->reqHeaders = (struct RequestHeader*)malloc(sizeof(struct RequestHeader) * HeaderSize);
	return request;
}

void httpRequestReset(struct HttpRequest* req) {
	if (req != NULL) {
		httpRequestResetEx(req);
		if (req->reqHeaders != NULL) {
			for (int i = 0; i < req->reqHeadersNum; i++) {
				free(req->reqHeaders[i].key);
				free(req->reqHeaders[i].value);
			}
		}
	}
	req->curState = ParseReqLine;
	req->method = NULL;
	req->version = NULL;
	req->url = NULL;
	req->reqHeadersNum = 0;

}
void httpRequestResetEx(struct HttpRequest* req) {
	free(req->url);
	free(req->method);
	free(req->version);
}

void httpRequestDestroy(struct HttpRequest* req) {
	if (req != NULL) {
		httpRequestResetEx(req);
		if (req->reqHeaders != NULL) {
			for (int i = 0; i < req->reqHeadersNum; i++) {
				free(req->reqHeaders[i].key);
				free(req->reqHeaders[i].value);
			}
		}
	}
}

enum HttpRequestState HttpRequestState(struct HttpRequest* request) {

	return request->curState;
}

void httpRequestAddHeader(struct HttpRequest* request, const char* key, const char* value){
	request->reqHeaders[request->reqHeadersNum].key = key;
	request->reqHeaders[request->reqHeadersNum].value = value;
	request->reqHeadersNum++;
}

char* httpRequestGetHeader(struct HttpRequest* request, const char* key) {
	if (request != NULL) {
		for (int i = 0; i < request->reqHeadersNum; i++) {
			if (strncasecmp(request->reqHeaders[i].key, key, strlen(key)) == 0) {
				return request->reqHeaders[i].value;
			}
		}
	}
	return NULL;
}
char* splitRequestLine(const char* start, const char* end, const char* sub, char** ptr) {
	char* space = end;
	if (sub != NULL) {
		space = (char*)memmem(start, end - start, sub, strlen(sub));
		assert(space != NULL);
	}
	
	int length = space - start;
	char* tmp = (char*)malloc(length + 1);
	strncpy(tmp, start, length);
	tmp[length] = '\0';
	*ptr = tmp;
	return space + 1;
}
bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* readBuf) {
	//读出请求行 保存字符串结束地址
	char* end = bufferFindCRLF(readBuf);
	//保存字符起始地址
	char* start = readBuf->data + readBuf->readPos;
	//请求行总长度
	int lineSize = end - start;

	if (lineSize) {
		//get /xxx/xx.txt http/1.1
		//请求方式
		start = splitRequestLine(start, end, " ", &request->method);
		//请求的静态资源
		start = splitRequestLine(start, end, " ", &request->url);
		//http 版本
		start = splitRequestLine(start, end, " ", &request->version);
		//为解析请求头做准备
		readBuf->readPos += lineSize;
		readBuf->readPos += 2;
		//修改状态
		request->curState = ParseReqHeaders;
		return true;
	}
	return false;
}

bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf) {
	char* end = bufferFindCRLF(readBuf);
	if (end != NULL) {
		char* start = readBuf->data + readBuf->readPos;
		int lineSize = end - start;
		char* middle = (char*)memmem(start, lineSize, ": ", 2);
		if (middle != NULL) {
			char* key = (char*)malloc(middle - start + 1);
			strncpy(key, start, middle - start);
			key[middle - start] = '\0';

			char* value = (char*)malloc(end - middle + 1);
			strncpy(key, middle + 2, end-middle - 2);
			value[end - middle - 2] = '\0';

			httpRequestAddHeader(request, key, value);

			//移动读数据的位置

			readBuf->readPos += lineSize;
			readBuf->readPos += 2;
		}
		else {
			//请求行被解析完了，跳过空行
			readBuf->readPos += 2;
			//修改解析状态
			request->curState = ParseReqDone;
		}
		return true;
	}
	return false;
}

bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf,
	struct HttpResponse* response, struct Buffer* sendBuf, int socket) {
	bool flag = true;
	while (request->curState!=ParseReqDone) {
		switch (request->curState)
		{
		case ParseReqLine:
			flag = parseHttpRequestLine(request, readBuf);
			break;
		case ParseReqHeaders:
			flag = parseHttpRequestHeader(request, readBuf);
			break;
		case ParseReqBody:
			break;
		default:
			break;
		}
		if (!flag) {
			return flag;
		}
		//判断是否解析完毕了，如果完毕了，需要准备回复的数据
		if (request->curState == ParseReqDone) {
			//根据解析出的原始数据，对客户端的请求做出处理
			processHttpRequest(request, response);
			//组织响应数据并发给客户端
			httpResponsePrepareMsg(response, sendBuf, socket);
			
		}
	}
	request->curState = ParseReqLine;
	return flag;
}

bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response) {
	if (strcasecmp(request->method, "get") != 0) {
		return -1;
	}
	//decodeMsg(request->url,request->url)
	//处理客户端请求的静态资源（目录或文件）
	char* file = NULL;
	if (strcmp(request->url, "/") == 0) {
		file = "./";
	}
	else {
		file = request->url + 1;
	}
	//获取文件属性
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1) {
		//文件不存在 回复 404
		strcpy(response->fileName, "404.html");
		response->statusCode = NotFound;
		strcpy(response->statusMsg, "Not Found");
		//响应头
		httpResponseAddHeader(response, "Content - type", getFiletype(".html"));
		response->sendDataFunc = sendFile;
		return 0;
	}
	strcpy(response->fileName, file);
	response->statusCode = OK;
	strcpy(response->statusMsg, "OK");
	if (S_ISDIR(st.st_mode)) {
		//把这个目录中的内容发送给客户端
		//响应头
		httpResponseAddHeader(response, "Content - type", getFiletype(".html"));
		response->sendDataFunc = sendDir;
	}
	else {
		//把这个文件的内容发送给客户端
		//响应头
		char tmp[12] = { 0 };
		sprintf(tmp, "%ld", st.st_size);
		httpResponseAddHeader(response, "Content - type", getFiletype(file));
		httpResponseAddHeader(response, "Content - length", tmp);
		response->sendDataFunc = sendFile;
	}
	return 0;
}


const char* getFiletype(const char* name)
{
	//a.jpg a.mp4 a.html
	//自右向左查找 '.' 字符，如不存在返回 NULL
	const char* dot = strrchr(name, '.');
	if (dot == NULL)
		return "text/plain; charset=utf-8"; //纯文本
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";
	return "text/plain; charset=utf-8";
}
int sendFile(const char* fileName, struct Buffer* sendBuf, int cfd) {
	//打开文件
	int fd = open(fileName, O_RDONLY);
	assert(fd > 0);
	while (1) {
		char buf[1024];
		int len = read(fd, buf, sizeof(buf));
		if (len > 0) {
			bufferAppendDate(sendBuf, buf, len);
#ifndef MSG_SEND_AUTO
			bufferSendData(sendBuf, cfd);
#endif
		}
		else if (len == 0) {
			break;
		}
		else {
			close(fd);
			return 0;
		}
	}
	return 1;
}
int sendDir(const char* dirName, struct Buffer* sendBuf, int cfd) {
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
	struct dirent** namelist;
	int num = scandir(dirName, &namelist, NULL, alphasort);
	for (int i = 0; i < num; i++)
	{
		char* name = namelist[i]->d_name;
		if (name[0] == '.') continue;
		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName, name);
		stat(subPath, &st);
		if (S_ISDIR(st.st_mode))
		{
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else
		{
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
		bufferSendData(sendBuf, cfd);
#endif
		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	sprintf(buf, "</table></body></html>");
	bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
	bufferSendData(sendBuf, cfd);
#endif // !MSG_SEND_AUTO

	
	free(namelist);
	return 0;
}