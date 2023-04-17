#include "EventLoop.h"
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Log.h"


struct EventLoop* eventLoopInit() {
	return eventLoopInitEx(NULL);
}
//写数据
void taskWakeup(struct EventLoop* evLoop) {
	const char* msg = "我是要成为海贼王的男人!!!";
	write(evLoop->socketPair[0], msg, strlen(msg));
}
//读数据
int readLocalMessage(void* arg) {
	struct EventLoop* evLoop = (struct EventLoop*)arg;
	char buf[256];
	read(evLoop->socketPair[1], buf, sizeof(buf));
	return 0;
}
struct EventLoop* eventLoopInitEx(const char* threadName) {
	struct EventLoop* evLoop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
	evLoop->isQuit = false;
	evLoop->threadID = pthread_self();
	pthread_mutex_init(&evLoop->mutex, NULL);
	strcpy(evLoop->threadName, threadName == NULL ? "MainThread" : threadName);
	evLoop->dispatcher = &EpollDispatcher;
	evLoop->dispatcherData = evLoop->dispatcher->init();
	//链表
	evLoop->head = evLoop->tail = NULL;
	//map
	evLoop->channelMap = channelMapInit(128);
	int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair);
	if (ret == -1) {
		perror("socketpair");
		exit(0);
	}
	Debug("channelInit??");
	struct Channel* channel = channelInit(evLoop->socketPair[1], ReadEvent,
		readLocalMessage, NULL, NULL, evLoop);
	Debug("channelInit!!eventLoopAddTask??");
	eventLoopAddTask(evLoop, channel, ADD);
	Debug("eventLoopAddTask!!");
	return evLoop;
}

int eventLoopRun(struct EventLoop* evLoop) {
	assert(evLoop != NULL);
	//取出事件分发和检测模型
	struct Dispatcher* dispatcher = evLoop->dispatcher;
	//比较线程 ID 是否正常
	if (evLoop->threadID != pthread_self()) {
		return -1;
	}
	//循环进行事件处理
	while (!evLoop->isQuit) {
		dispatcher->dispatch(evLoop, 2);
		eventLoopProcessTask(evLoop);
	}
	return 0;
}

int eventActivate(struct EventLoop* evLoop, int fd, int event) {
	if (fd < 0 || evLoop == NULL) {
		return -1;
	}
	struct Channel* channel = evLoop->channelMap->list[fd];
	assert(channel->fd == fd);
	if (event & ReadEvent && channel->readCallback) {
		channel->readCallback(channel->arg);
	}
	if (event & WriteEvent && channel->writeCallback) {
		channel->writeCallback(channel->arg);
		
	}

	return 0;
}

int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type) {
	//加锁，保护共享资源
	pthread_mutex_lock(&evLoop->mutex);
	//创建新节点
	struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof(struct ChannelElement));
	node->channel = channel;
	node->type = type;
	node->next = NULL;
	//链表为空
	if (evLoop->head == NULL) {
		evLoop->head = evLoop->tail = node;
	}
	else {
		evLoop->tail->next = node;
		evLoop->tail = node;
	}
	pthread_mutex_unlock(&evLoop->mutex);
	if (evLoop->threadID == pthread_self()) {//这样为什么是子线程
		//当前子线程
		
		eventLoopProcessTask(evLoop);
	}
	else {
		//主线程
		taskWakeup(evLoop);
	}
	return 0;
}

int eventLoopProcessTask(struct EventLoop* evLoop) {
	pthread_mutex_lock(&evLoop->mutex);
	//取出头节点
	struct ChannelElement* head = evLoop->head;
	Debug("eventLoop begin...");
	while (head != NULL) {
		struct Channel* channel = head->channel;
		if (head->type == ADD) {
			//添加
			eventLoopAdd(evLoop, channel);
		}
		else if (head->type == DELETE) {
			//删除
			eventLoopRemove(evLoop, channel);

		}
		else if (head->type == MODIFY) {
			//更新
			eventLoopModify(evLoop, channel);

		}
		struct ChannelElement* tmp = head;
		head = head->next;
		free(tmp);
	}
	evLoop->head = evLoop->tail = NULL;
	Debug("eventLoop end...");
	pthread_mutex_unlock(&evLoop->mutex);
	return 0;
}

int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel) {
	Debug("ADD!!!");
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size) {
		Debug("no memory!!");
		//没有足够空间存储键值对
		if (!makeMapRoom(channelMap, fd, sizeof(struct Channel*))) {
			return -1;
		}
		Debug("have memory!!");
	}
	Debug("check??");
	if (channelMap->list[fd] == NULL) {
		Debug("check in!!");
		channelMap->list[fd] = channel;
		evLoop->dispatcher->add(channel, evLoop);
	}
	Debug("check out!!");
	return 0;
}

int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel) {
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size) {
		return -1;
	}
	int ret = evLoop->dispatcher->remove(channel, evLoop);
	return ret;
}

int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel) {
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size) {
		return -1;
	}
	int ret = evLoop->dispatcher->modify(channel, evLoop);
	return ret;
}

int destroyChannel(struct EventLoop* evLoop, struct Channel* channel) {
	//删除channel和fd 的对应关系
	evLoop->channelMap->list[channel->fd] = NULL;
	//关闭 fd
	close(channel->fd);
	//释放channel
	free(channel);
	return 0;
}
