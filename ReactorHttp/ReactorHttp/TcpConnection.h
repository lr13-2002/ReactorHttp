#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
//#define MSG_SEND_AUTO
struct TcpConnection {
	struct EventLoop* evLoop;
	struct Channel* channel;
	struct Buffer* readBuf;
	struct Buffer* writeBuf;
	struct HttpRequest* request;
	struct HttpResponse* response;
	char name[32];

};

//≥ı ºªØ
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evLoop);

int tcpConnectionDestroy(void* arg);