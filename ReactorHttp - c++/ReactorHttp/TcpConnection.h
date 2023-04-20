#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
//#define MSG_SEND_AUTO
class TcpConnection {
public:
	TcpConnection(int fd, EventLoop* evloop);
	~TcpConnection();
	static int destroy(void* arg);
	static int processRead(void* arg);
	static int processWrite(void* arg);
private:
	EventLoop* m_evLoop;
	Channel* m_channel;
	Buffer* m_readBuf;
	Buffer* m_writeBuf;
	//http 协议
	HttpRequest* m_request;
	HttpResponse* m_response;
	string name;

};
