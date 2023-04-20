#include "EventLoop.h"
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include "Log.h"
#include "EpollDispatcher.h"


EventLoop::EventLoop() : EventLoop(string()) {

}
EventLoop::~EventLoop() {
}
//写数据
void EventLoop::taskWake() {
	const char* msg = "我是要成为海贼王的男人!!!";
	write(socketPair[0], msg, strlen(msg));
}
//读数据
int EventLoop::readMessage() {
	char buf[256];
	read(socketPair[1], buf, sizeof(buf));
	return 0;
}
EventLoop::EventLoop(const string threadName) {
	dispatcher = new EpollDispatcher(this);
	isQuit = true;
	m_channelMap.clear();
	this->threadName = threadName == string() ? "MainThread" : threadName;
	threadID = this_thread::get_id();
	int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socketPair);
	if (ret == -1) {
		perror("socketpair");
		exit(0);
	}
	auto obj = bind(&EventLoop::readMessage, this);
	Channel* channel = new Channel(socketPair[1], FDEvent::ReadEvent,
		obj, nullptr, nullptr, this);
	//将任务放进任务队列
	AddTask(channel, ElemType::ADD);
}

int EventLoop::Run() {
	isQuit = false;
	//比较线程 ID 是否正常
	if (threadID != this_thread::get_id()) {
		return -1;
	}
	//循环进行事件处理
	while (!isQuit) {
		dispatcher->dispatch();
		ProcessTask();
	}
	return 0;
}

int EventLoop::eventActivate(int fd, int event) {
	if (fd < 0 ) {
		return -1;
	}
	if (!m_channelMap.count(fd)) {
		while (1) {
			debug("没找到的文件描述符 : %d", fd);
		}
	}
	Channel* channel = m_channelMap[fd];
	assert(channel->get_Socket() == fd);
	debug("正在处理的文件 : %d", fd);
	if (event & (int)FDEvent::ReadEvent && channel->readCallback) {
		channel->readCallback(const_cast<void*>(channel->get_Arg()));
	}
	if (event & (int)FDEvent::WriteEvent && channel->writeCallback) {
		channel->writeCallback(const_cast<void*>(channel->get_Arg()));
	}

	return 0;
}

int EventLoop::AddTask(Channel* channel, ElemType type) {
	//加锁，保护共享资源
	m_mutex.lock();
	//创建新节点
	ChannelElement* node = new ChannelElement;
	node->channel = channel;
	node->type = type;
	//链表为空
	q.push(node);
	m_mutex.unlock();
	if (threadID == this_thread::get_id() ) {//这样为什么是子线程
		//当前子线程
		debug(threadName.data());
		ProcessTask();
	}
	else {
		//主线程
		taskWake();
	}
	return 0;
}

int EventLoop::ProcessTask() {
	//取出头节点
	while (!q.empty()) {
		m_mutex.lock();
		ChannelElement* node = q.front(); q.pop();
		m_mutex.unlock();
		Channel* channel = node->channel;

		if (node->type == ElemType::ADD) {
			//添加

			Add(channel);
		}
		else if (node->type == ElemType::DELETE) {
			//删除
			Remove(channel);

		}
		else if (node->type == ElemType::MODIFY) {
			//更新
			Modify(channel);

		}
		delete node;
	}
	return 0;
}

int EventLoop::Add(Channel* channel) {
	int fd = channel->get_Socket();
	debug("添加的文件 : %d??", fd);
	if (!m_channelMap.count(fd)) {
		m_channelMap.insert(make_pair(fd, channel));
		dispatcher->setChannel(channel);
		debug("添加的文件 : %d!!", fd);
		int ret = dispatcher->add();
		return ret;
	}
	return -1;
}

int EventLoop::Remove(Channel* channel) {
	int fd = channel->get_Socket();
	if (!m_channelMap.count(fd)) {
		return -1;
	}
	dispatcher->setChannel(channel);
	int ret = dispatcher->remove();
	return ret;
}

int EventLoop::Modify(Channel* channel) {
	int fd = channel->get_Socket();
	if (!m_channelMap.count(fd)) {
		return -1;
	}
	dispatcher->setChannel(channel);
	int ret = dispatcher->modify();
	return ret;
}

int EventLoop::freeChannel(Channel* channel) {
	//删除channel和fd 的对应关系
	auto it = m_channelMap.find(channel->get_Socket());
	if (it != m_channelMap.end()) {
		m_channelMap.erase(it);
		close(channel->get_Socket());
		delete channel;
	}
	return 0;
}
