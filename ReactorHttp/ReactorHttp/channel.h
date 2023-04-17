#pragma once
#include<stdio.h>
#include<stdbool.h>
typedef int(*handleFunc)(void* arg);
struct Channel {
	//文件描述符
	int fd;
	//事件
	int events;
	//回调函数
	handleFunc readCallback;
	handleFunc writeCallback;
	handleFunc destroyCallback;
	//回调函数的参数
	void* arg;
};
enum FDEvent {
	TimeOur = 0x01,
	ReadEvent = 0x02,
	WriteEvent = 0x04
};
//初始化一个 Channel
struct Channel* channelInit(int fd, int events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg);
//修改 fd 的写事件
void writeEventEnable(struct Channel* channel, bool flag);
//判断是否需要检测文件描述符的写事件
bool isWriteEventEnable(struct Channel* channel);