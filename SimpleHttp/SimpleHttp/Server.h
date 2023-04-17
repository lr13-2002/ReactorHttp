#pragma once
//初始化监听的套接字
int initListenLd(unsigned short port);
//启动 epoll
int epollRun(int lfd);
//和客户端建立新的连接
//int acceptClient(int lfd, int efd);
void* acceptClient(void* arg);
//接收http的请求消息
//int recvHttpRequest(int cfd, int efd);
void* recvHttpRequest(void* arg);
//解析http的请求行
int parseRequestLine(const char* line, int cfd);
//发送文件
int sendFile(const char* fileName, int cfd);
//发送响应头（状态行和响应头）
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length);

const char* getFiletype(const char* name);

int sendDir(const char* dirName, int cfd);

int hexToDec(char c);
