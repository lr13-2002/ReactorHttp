#include "TcpConnection.h"
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>

int processRead(void* arg) {
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	//接收数据
	int count = bufferSocketRead(conn->readBuf, conn->channel->fd);
	if (count > 0) {
		//接收到了 http 请求 ，解析 http 请求
		int socket = conn->channel->fd;
#ifndef  MSG_SEND_AUTO
		writeEventEnable(conn->channel, true);
		eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
#endif //  MSG_SEND_AUTO
		int flag = parseHttpRequest(conn->request, conn->readBuf, conn->response,
			conn->writeBuf, socket);
		if (!flag) {
			//解析失败，回复一个简单的html
			char* errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			bufferAppendString(conn->writeBuf, errMsg);
		}
	}
	else {
#ifdef  MSG_SEND_AUTO
		//断开连接
		eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
	}
#ifndef  MSG_SEND_AUTO
	//断开连接
	eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
	return 0;
}

int processWrite(void* arg) {
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	//发送数据
	int count = bufferSendData(conn->writeBuf, conn->channel->fd);
	if (count > 0) {
		//判断数据是否被全部发出去
		if (bufferReadableSize(conn->writeBuf) == 0) {
			//不再检测写事件
			writeEventEnable(conn->channel, false);
			//修改 dispatcher 检测的集合
			eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
			//删除这个节点
			eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
		}
	}
	return 0;
}

struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evloop) {
	struct TcpConnection* conn = (struct TcpConnection*)malloc(sizeof(struct TcpConnection));
	conn->evLoop = evloop;
	conn->readBuf = bufferInit(10240);
	conn->writeBuf = bufferInit(10240);
	conn->request = httpRequestInit();
	conn->response = httpResponseInit();
	sprintf(conn->name, "Connect-%d", fd);
	conn->channel = channelInit(fd, ReadEvent, processRead, processWrite, tcpConnectionDestroy, conn);
	eventLoopAddTask(evloop, conn->channel, ADD);
	return conn;
}

int tcpConnectionDestroy(void* arg) {
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	if (conn != NULL) {
		if (conn->readBuf && bufferReadableSize(conn->readBuf) == 0 &&
			conn->writeBuf && bufferReadableSize(conn->writeBuf) == 0) {
			destroyChannel(conn->evLoop, conn->channel);
			bufferDestroy(conn->readBuf);
			bufferDestroy(conn->writeBuf);
			httpRequestDestroy(conn->request);
			httpResponseDestroy(conn->response);
			free(conn);
		}
	}
	return 0;
}
