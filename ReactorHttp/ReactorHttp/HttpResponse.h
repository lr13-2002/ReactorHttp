#pragma once
#include "Buffer.h"
//����״̬��ö��
enum HttpStatusCode {
	Unknown = 0,
	OK = 200,
	MovedPermanently = 301,
	MovedTemporarily = 302,
	BadRequest = 400,
	NotFound = 404
};

//������Ӧ�ṹ��
struct ResponseHeader {
	//״̬�У�״̬�룬״̬����
	char key[32];
	char value[128];
};
//����һ������ָ�룬������֯Ҫ�ָ��Ŀͻ��˵����ݿ�
typedef int(*responseBody)(const char* fileName, struct Buffer* sendBuf, int socket);
//����ṹ��
struct HttpResponse {
	//״̬�У�״̬�룬״̬����
	enum HttpStatusCode statusCode;
	char statusMsg[128];
	char fileName[128];
	//��Ӧͷ-��ֵ��
	struct ResponseHeader* headers;
	int headerNum;
	responseBody sendDataFunc;
};
//��ʼ��
struct HttpResponse* httpResponseInit();
//����
void httpResponseDestroy(struct HttpResponse* response);
//������Ӧͷ
void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* value);
//��֯ http ��Ӧ����
void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket);