#pragma once
#include "Dispatcher.h"
#include "channel.h"
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <thread>
#include <mutex>
#include <map>
using namespace std;

enum class ElemType{
	ADD, 
	DELETE, 
	MODIFY
};
class Channel;
//定义任务队列的节点
struct ChannelElement{
	ElemType type;
	Channel* channel;
};
class Dispatcher;
class EventLoop {
public:
	EventLoop();
	EventLoop(const string threadName);
	~EventLoop();

	//启动反应堆模型
	int Run();
	//处理待激活的文件 fd
	int eventActivate(int fd, int event);
	//添加任务到任务队列
	int AddTask(Channel* channel, ElemType type);
	//处理任务队列中的任务
	int ProcessTask();
	//处理dispatcher中的节点
	int Add(Channel* channel);
	int Remove(Channel* channel);
	int Modify(Channel* channel);
	//释放 channel
	int freeChannel(Channel* channel);

	int readMessage();
	// 返回线程ID
	inline thread::id getThreadID() {
		return threadID;
	}
private:
	void taskWake();
	bool isQuit;
	Dispatcher* dispatcher;
	//任务队列
	queue<ChannelElement*>q;
	//map
	map<int, Channel*> m_channelMap;
	//线程 id ，name
	thread::id threadID;
	string threadName;
	mutex m_mutex;
	int socketPair[2];
};