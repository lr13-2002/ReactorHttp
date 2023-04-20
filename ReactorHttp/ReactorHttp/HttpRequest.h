#pragma once
#include <stdbool.h>
#include "Buffer.h"
#include "HttpResponse.h"
//请求头键值对
struct RequestHeader {
	char* key;
	char* value;
};
//当前的解析状态
enum HttpRequestState{
	ParseReqLine,
	ParseReqHeaders,
	ParseReqBody,
	ParseReqDone
};
//定义 http 请求结构体
struct HttpRequest {
	char* method;
	char* url;
	char* version;
	struct RequestHeader* reqHeaders;
	int reqHeadersNum;
	enum HttpRequestState curState;
};
//初始化
struct HttpRequest* httpRequestInit();

//重置
void httpRequestReset(struct HttpRequest* req);
void httpRequestResetEx(struct HttpRequest* req);
void httpRequestDestroy(struct HttpRequest* req);
//获取处理状态
enum HttpRequestState httpRequestState(struct HttpRequest* request);
//添加请求头
void httpRequestAddHeader(struct HttpRequest* request, const char* key, const char* value);
//根据 key 得到请求头的 value
char* httpRequestGetHeader(struct HttpRequest* request, const char* key);
//解析请求行
bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* readBuf);
//解析请求头
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf);
//解析 http 请求协议
bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf, 
	struct HttpResponse* response, struct Buffer* sendBuf, int socket);
//处理基于 get 的 http 请求协议
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response);

const char* getFileType(const char* name);
//
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd);

void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd);
void decodeMsg(char* to, char* from);
int hexToDec(char c);
