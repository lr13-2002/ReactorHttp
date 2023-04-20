#include "TcpConnection.h"
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>

int TcpConnection::processRead(void* arg) {
	TcpConnection* conn = (TcpConnection*)arg;
	//接收数据
	int socket = conn->m_channel->get_Socket();
	int count = conn->m_readBuf->SocketRead(socket);
	if (count > 0) {
		//接收到了 http 请求 ，解析 http 请求
#ifdef  MSG_SEND_AUTO
		conn->channel->writeEventEnable(true);
		conn->evLoop->AddTask(conn->channel, ElemType::MODIFY);
#endif //  MSG_SEND_AUTO
		int flag = conn->m_request->parseRequest(conn->m_readBuf, conn->m_response,
			conn->m_writeBuf, socket);
		if (!flag) {
			//解析失败，回复一个简单的html
			string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			conn->m_writeBuf->AppendString(errMsg);
		}
	}
	else {
#ifdef  MSG_SEND_AUTO
		//断开连接
		conn->evLoop->AddTask(conn->channel, ElemType::DELETE);
#endif
	}
#ifndef  MSG_SEND_AUTO
	//断开连接
	conn->m_evLoop->AddTask(conn->m_channel, ElemType::DELETE);
#endif
	return 0;
}

int TcpConnection::destroy(void* arg) {
	TcpConnection* conn = (TcpConnection*)arg;
	if (conn != nullptr) {
		delete conn;
	}
	return 0;
}
int TcpConnection::processWrite(void* arg) {
	TcpConnection* conn = (TcpConnection*)arg;
	//发送数据
	int count = conn->m_writeBuf->SendData(conn->m_channel->get_Socket());
	if (count > 0) {
		//判断数据是否被全部发出去
		if (conn->m_writeBuf->ReadableSize() == 0) {
			//不再检测写事件
			conn->m_channel->writeEventEnable(false);
			//修改 dispatcher 检测的集合
			conn->m_evLoop->AddTask(conn->m_channel, ElemType::MODIFY);
			//删除这个节点
			conn->m_evLoop->AddTask(conn->m_channel, ElemType::DELETE);
		}
	}
	return 0;
}

TcpConnection::TcpConnection(int fd, EventLoop* evloop) {
	m_evLoop = evloop;
	m_readBuf = new Buffer(10240);
	m_writeBuf = new Buffer(10240);
	m_request = new HttpRequest;
	m_response = new HttpResponse;
	name = "Connect-" + to_string(fd);
	m_channel = new Channel(fd, FDEvent::ReadEvent, processRead, processWrite, destroy, this);
	evloop->AddTask(m_channel, ElemType::ADD);
}

TcpConnection::~TcpConnection() {
	if (m_readBuf && m_readBuf->ReadableSize() == 0 && m_writeBuf && m_writeBuf->ReadableSize() == 0) {
		delete m_channel;
		delete m_readBuf;
		delete m_writeBuf;
		delete m_request;
		delete m_response;
		m_evLoop->freeChannel(m_channel);
	}
}
