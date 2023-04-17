#include "TcpServer.h"
#include "Log.h"
#include "TcpConnection.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
struct TcpServer* tcpServerInit(unsigned short port, int threadNum) {
	struct TcpServer* tcp = (struct TcpServer*)malloc(sizeof(struct TcpServer));
	tcp->threadNumn = threadNum;
	Debug("Listen??");
	tcp->listener = listenerInit(port);
	Debug("Listen!! eventLoop??");
	tcp->mainLoop = eventLoopInit();
	Debug("eventLoop!!  threadpoll??");
	tcp->threadPool = threadPoolInit(tcp->mainLoop, threadNum);
	Debug("threadpoll!!");
	return tcp;
}

struct Listener* listenerInit(unsigned short port) {
	struct Listener* listener = (struct Listener*)malloc(sizeof(struct Listener));
	// 1. 创建监听的 fd
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("lfd");
		return NULL;
	}
	// 2, 设置端口复用
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (ret == -1) {
		perror("setsockopt");
		return NULL;
	}
	// 3. 绑定
	struct sockaddr_in addr;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof addr);
	if (ret == -1) {
		perror("bind");
		return NULL;
	}
	// 4. 设置监听
	ret = listen(lfd, 128);
	if (ret == -1) {
		perror("listen");
		return NULL;
	}
	// 返回 fd
	listener->lfd = lfd;
	listener->port = port;
	return listener;
}
int acceptConnection(void* arg) {

	struct TcpServer* server = (struct TcpServer*)arg;
	//和客户端建立连接
	int cfd = accept(server->listener->lfd, NULL, NULL);
	//取出线程池中得某个子线程
	struct EventLoop* evLoop = takeWorkerEventLoop(server->threadPool);
	//将 cfd 放到 TcpConnection 中处理
	tcpConnectionInit(cfd, evLoop);
	return 0;
}
void tcpServerRun(struct TcpServer* server) {
	//启动线程池
	Debug("Runing threadPoll bigin...");
	threadPoolRun(server->threadPool);
	Debug("Runing threadPoll end...");
	//初始化一个channel实例
	struct Channel* channel = channelInit(server->listener->lfd, ReadEvent,
		acceptConnection, NULL, NULL, server);
	//添加检测得任务
	Debug("Runing AddTask...");
	eventLoopAddTask(server->mainLoop, channel, ADD);
	Debug("Runing AddTask!");
	//启动反应堆模型
	Debug("Runing eventLoop begin...");
	eventLoopRun(server->mainLoop);
	Debug("Runing eventLoop end...");
}
