#include "Dispatcher.h"
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "Log.h"
#define Max 520

struct EpollData {
	int epfd;
	struct epoll_event* events;
};

static void* epollInit();
static int epollAdd(struct Channel* channel, struct EventLoop* evLoop);
static int epollRemove(struct Channel* channel, struct EventLoop* evLoop);
static int epollModify(struct Channel* channel, struct EventLoop* evLoop);
static int epollCtl(struct Channel* channel, struct EventLoop* evLoop, int op);
static int epollDispatch(struct EventLoop* evLoop, int timeout);
static int epollclear(struct EventLoop* evLoop);

struct Dispatcher EpollDispatcher = {
	epollInit,
	epollAdd,
	epollRemove,
	epollModify,
	epollDispatch,
	epollclear
};
static int epollCtl(struct Channel* channel, struct EventLoop* evLoop, int op) {
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	struct epoll_event ev;
	ev.data.fd = channel->fd;
	int events = 0;
	if (channel->events & ReadEvent) {
		events |= EPOLLIN;
	}
	if (channel->events & WriteEvent) {
		events |= EPOLLOUT;
	}
	ev.events = events;
	int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
	return ret;
}
static void* epollInit() {
	struct EpollData* data = (struct EpollData*)malloc(sizeof(struct EpollData));
	data->epfd = epoll_create(10);
	if (data->epfd == -1) {
		perror("epoll_create");
		exit(0);
	}
	data->events = (struct epoll_event*)calloc(Max, sizeof(struct epoll_event));
	return data;
}
static int epollAdd(struct Channel* channel, struct EventLoop* evLoop) {
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_ADD);
	if (ret == -1) {
		perror("epoll_ctl add");
		exit(0);
	}
	return 1;
}
static int epollRemove(struct Channel* channel, struct EventLoop* evLoop) {
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_DEL);
	if (ret == -1) {
		perror("epoll_ctl del");
		exit(0);
	}
	//通过 channel 释放对应的 TcpConnection 资源
	channel->destroyCallback(channel->arg);
	return 1;
}
static int epollModify(struct Channel* channel, struct EventLoop* evLoop) {
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_MOD);
	if (ret == -1) {
		perror("epoll_ctl mod");
		exit(0);
	}
	return 1;
}
static int epollDispatch(struct EventLoop* evLoop, int timeout) {
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;

	Debug("%d %s epollwait..", evLoop->threadID, evLoop->threadName);
	int count = epoll_wait(data->epfd, data->events, Max, timeout);
	Debug("%d %s epollwait!! count = %d", evLoop->threadID, evLoop->threadName, count);
	for (int i = 0; i < count; i++) {
		int events = data->events[i].events;
		int fd = data->events[i].data.fd;
		if (events & EPOLLERR || events & EPOLLHUP) {
			//对方断开了连接 删除 fd
			//epollRemove(Channel, evloop);
			continue;
		}
		if (events & EPOLLIN) {
			eventActivate(evLoop, fd, ReadEvent);
		}
		if (events & EPOLLOUT) {
			eventActivate(evLoop, fd, WriteEvent);
		}
	}
}
static int epollclear(struct EventLoop* evLoop) {
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	free(data->events);
	close(data->epfd);
	free(data);
	return 1;
}