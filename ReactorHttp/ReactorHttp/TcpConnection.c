#include "TcpConnection.h"
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>

int processRead(void* arg) {
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	//��������
	int count = bufferSocketRead(conn->readBuf, conn->channel->fd);
	if (count > 0) {
		//���յ��� http ���� ������ http ����
		int socket = conn->channel->fd;
#ifndef  MSG_SEND_AUTO
		writeEventEnable(conn->channel, true);
		eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
#endif //  MSG_SEND_AUTO
		int flag = parseHttpRequest(conn->request, conn->readBuf, conn->response,
			conn->writeBuf, socket);
		if (!flag) {
			//����ʧ�ܣ��ظ�һ���򵥵�html
			char* errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			bufferAppendString(conn->writeBuf, errMsg);
		}
	}
	else {
#ifdef  MSG_SEND_AUTO
		//�Ͽ�����
		eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
	}
#ifndef  MSG_SEND_AUTO
	//�Ͽ�����
	eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
	return 0;
}

int processWrite(void* arg) {
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	//��������
	int count = bufferSendData(conn->writeBuf, conn->channel->fd);
	if (count > 0) {
		//�ж������Ƿ�ȫ������ȥ
		if (bufferReadableSize(conn->writeBuf) == 0) {
			//���ټ��д�¼�
			writeEventEnable(conn->channel, false);
			//�޸� dispatcher ���ļ���
			eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
			//ɾ������ڵ�
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
