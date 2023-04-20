#include "TcpServer.h"
#include "Log.h"
#include "TcpConnection.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
TcpServer::TcpServer(unsigned short port, int threadNum) {
	m_port = port;
	m_mainLoop = new EventLoop();
	m_threadPool = new ThreadPool(m_mainLoop, threadNum);
	m_threadNumn = threadNum;
	setListener();
}

void TcpServer::setListener() {
	// 1. 创建监听的 fd
	m_lfd = socket(AF_INET, SOCK_STREAM, 0);
	debug("!!!!! m_fd : %d", m_lfd);
	if (m_lfd == -1) {
		perror("lfd");
		exit(0);
	}
	// 2, 设置端口复用
	int opt = 1;
	int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (ret == -1) {
		perror("setsockopt");
		exit(0);
	}
	// 3. 绑定
	struct sockaddr_in addr;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_port);
	ret = bind(m_lfd, (struct sockaddr*)&addr, sizeof addr);
	if (ret == -1) {
		perror("bind");
		exit(0);
	}
	// 4. 设置监听
	ret = listen(m_lfd, 128);
	if (ret == -1) {
		perror("listen");
		exit(0);
	}
}
int TcpServer::acceptConnection(void* arg) {

	TcpServer* server = (TcpServer*)arg;
	//和客户端建立连接
	int cfd = accept(server->m_lfd, NULL, NULL);
	//取出线程池中得某个子线程
	//EventLoop* evLoop = server->m_threadPool->takeWorkerEventLoop();
	//将 cfd 放到 TcpConnection 中处理
	new TcpConnection(cfd, server->m_mainLoop);
	debug("connection!!");
	return 0;
}
void TcpServer::Run() {
	//启动线程池
	m_threadPool->Run();
	//初始化一个channel实例
	Channel* channel = new Channel(m_lfd, FDEvent::ReadEvent, acceptConnection, nullptr, nullptr, this);
	//添加检测得任务
	m_mainLoop->AddTask(channel, ElemType::ADD);
	//启动反应堆模型
	m_mainLoop->Run();
}
