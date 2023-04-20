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
HttpRequest::HttpRequest() {
	Reset();
}

void HttpRequest::Reset() {
	m_curState = PrecessState::ParseReqLine;
	m_method = "";
	m_version = "";
	m_url = "";
	m_reqHeaders.clear();

}

void HttpRequest::AddHeader(const string key, const string value){
	if (key.empty() || value.empty()) return;
	m_reqHeaders[key] = value;
}

string HttpRequest::GetHeader(const string key) {
	if (m_reqHeaders.count(key)) {
		return m_reqHeaders[key];
	}
	return "";
}
char* HttpRequest::splitRequestLine(const char* start, const char* end, const char* sub, function<void(string)> callback) {
	char* space = (char*)end;
	if (sub != NULL) {
		space = (char*)memmem(start, end - start, sub, strlen(sub));
		assert(space != NULL);
	}
	
	int length = space - start;
	callback(string(start, length));
	return space + 1;
}
bool HttpRequest::parseRequestLine(Buffer* readBuf) {
	//读出请求行 保存字符串结束地址
	char* end = readBuf->FindCRLF();
	//保存字符起始地址
	char* start = readBuf->data();
	//请求行总长度
	int lineSize = end - start;

	if (lineSize) {
		//get /xxx/xx.txt http/1.1
		//请求方式
		auto methodFuc = bind(&HttpRequest::setMethod, this, placeholders::_1);
		start = splitRequestLine(start, end, " ", methodFuc);
		//请求的静态资源
		auto urlFuc = bind(&HttpRequest::setUrl, this, placeholders::_1);
		start = splitRequestLine(start, end, " ", urlFuc);
		//http 版本
		auto versionFuc = bind(&HttpRequest::setVersion, this, placeholders::_1);
		start = splitRequestLine(start, end, " ", versionFuc);
		//为解析请求头做准备
		readBuf->readPosIncrease(lineSize + 2);
		//修改状态
		setState(PrecessState::ParseReqHeaders);
		return true;
	}
	return false;
}

bool HttpRequest::parseRequestHeader(Buffer* readBuf) {
	char* end =readBuf->FindCRLF();
	if (end != nullptr) {
		char* start = readBuf->data();
		int lineSize = end - start;
		char* middle = (char*)memmem(start, lineSize, ": ", 2);
		if (middle != nullptr) {
			int keyLen = middle - start + 1;
			int valueLen = end - middle + 1;
			if (keyLen > 0 && valueLen > 0) {
				string key(start, keyLen);
				string value(middle + 2, valueLen);
				AddHeader(key, value);
			}

			//移动读数据的位置

			readBuf->readPosIncrease(lineSize + 2);
		}
		else {
			//请求行被解析完了，跳过空行
			readBuf->readPosIncrease(2);
			//修改解析状态
			setState(PrecessState::ParseReqDone);
		}
		return true;
	}
	return false;
}

bool HttpRequest::parseRequest(Buffer* readBuf,
	HttpResponse* response, Buffer* sendBuf, int socket) {
	bool flag = true;
	while (getState() != PrecessState::ParseReqDone) {
		switch (getState())
		{
		case PrecessState::ParseReqLine:
			flag = parseRequestLine(readBuf);
			break;
		case PrecessState::ParseReqHeaders:
			flag = parseRequestHeader(readBuf);
			break;
		case PrecessState::ParseReqBody:
			break;
		default:
			break;
		}
		if (!flag) {
			return flag;
		}
		//判断是否解析完毕了，如果完毕了，需要准备回复的数据
		if (getState() == PrecessState::ParseReqDone) {
			//根据解析出的原始数据，对客户端的请求做出处理
			processRequest(response);
			//组织响应数据并发给客户端
			response->PrepareMsg(sendBuf, socket);
			
		}
	}
	m_curState = PrecessState::ParseReqLine;
	return flag;
}
int HttpRequest::hexToDec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}
string HttpRequest::decodeMsg(string msg) {
	string str = "";
	const char* from = msg.data();
	for (; *from != '\0'; ++from)
	{
		// isxdigit -> 判断字符是不是16进制格式，取值在0-f
		// Linux%E5%86%85%E6%A0%B8.jpg -> Linux内核.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// 将16进制的数 -> 十进制 将这个数只赋值给了字符 int -> char
			// R2==178
			// 将3个字符，变成了一个字符，这个字符就是原始数据
			str.append(1, hexToDec(from[1]) * 16 + hexToDec(from[2]));

			// 跳过from[1]和from[2] 因为在当前循环中已经处理过了
			from += 2;
		}
		else
		{
			//字符拷贝，赋值
			str.append(1, *from);
		}
	}
	str.append(1, '\0');
	return str;
}
bool HttpRequest::processRequest(HttpResponse* response) {
	if (m_method != "get") {
		return -1;
	}
	m_url = decodeMsg(m_url);
	//处理客户端请求的静态资源（目录或文件）
	const char* file = NULL;
	if (m_url == "/") {
		file = "./";
	}
	else {
		file = m_url.data() + 1;
	}
	//获取文件属性
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1) {
		//文件不存在 回复 404
		
		response->setStatusCode(StatusCode::NotFound);
		response->setFileName("404.html");
		//响应头
		response->AddHeader("Content - type", getFiletype(".html"));
		response->sendDataFunc = sendFile;
		return 0;
	}
	response->setFileName(file);
	response->setStatusCode(StatusCode::OK);
	if (S_ISDIR(st.st_mode)) {
		//把这个目录中的内容发送给客户端
		//响应头
		response->AddHeader("Content - type", getFiletype(".html"));
		response->sendDataFunc = sendDir;
	}
	else {
		//把这个文件的内容发送给客户端
		//响应头
		response->AddHeader("Content - type", getFiletype(file));
		response->AddHeader("Content - length", to_string(st.st_size));
		response->sendDataFunc = sendFile;
	}
	return 0;
}


const string HttpRequest::getFiletype(const string name) {
	//a.jpg a.mp4 a.html
	//自右向左查找 '.' 字符，如不存在返回 NULL
	const char* dot = strrchr(name.data(), '.');
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
void HttpRequest::sendFile(const string fileName, Buffer* sendBuf, int cfd) {
	//打开文件
	int fd = open(fileName.data(), O_RDONLY);
	assert(fd > 0);
	while (1) {
		char buf[1024];
		int len = read(fd, buf, sizeof(buf));
		if (len > 0) {
			sendBuf->AppendString(buf, len);
#ifndef MSG_SEND_AUTO
			sendBuf->SendData(cfd);
#endif
		}
		else if (len == 0) {
			break;
		}
		else {
			close(fd);
			perror("read");
			exit(0);
		}
	}
	close(fd);
}
int HttpRequest::sendDir(const string dirName, Buffer* sendBuf, int cfd) {
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
	struct dirent** namelist;
	int num = scandir(dirName.data(), &namelist, NULL, alphasort);
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
		sendBuf->AppendString(buf);
#ifndef MSG_SEND_AUTO
		sendBuf->SendData(cfd);
#endif
		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	sprintf(buf, "</table></body></html>");
	sendBuf->AppendString(buf);
#ifndef MSG_SEND_AUTO
	sendBuf->SendData(cfd);
#endif // !MSG_SEND_AUTO
	free(namelist);
	return 0;
}