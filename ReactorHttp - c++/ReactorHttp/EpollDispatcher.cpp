#include "EpollDispatcher.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "Log.h"
#define Max 520

EpollDispatcher::EpollDispatcher(EventLoop* evLoop) : Dispatcher(evLoop) {
	m_epfd = epoll_create(10);
	if (m_epfd == -1) {
		perror("epoll_create");
		exit(0);
	}
	m_events = new struct epoll_event[MaxNode];
	m_name = "Epoll";
}
int EpollDispatcher::epollctl(int op) {
	struct epoll_event ev;
	ev.data.fd = m_channel->get_Socket();
	if (m_channel->get_Event() & (int)FDEvent::ReadEvent) {
		ev.events |= EPOLLIN;
	}
	if (m_channel->get_Event() & (int)FDEvent::WriteEvent) {
		ev.events |= EPOLLOUT;
	}
	return epoll_ctl(m_epfd, op, m_channel->get_Socket(), &ev);
}
int EpollDispatcher::add() {
	int ret = epollctl(EPOLL_CTL_ADD);
	if (ret == -1) {
		perror("epoll_ctl_add");
	}
}
int EpollDispatcher::remove() {
	int ret = epollctl(EPOLL_CTL_DEL);
	if (ret == -1) {
		perror("epoll_ctl_del");
	}
}
int EpollDispatcher::modify() {
	int ret = epollctl(EPOLL_CTL_MOD);
	if (ret == -1) {
		perror("epoll_ctl_mod");
	}
}
int EpollDispatcher::dispatch(int timeout) {
	int count = epoll_wait(m_epfd, m_events, MaxNode, timeout);
	for (int i = 0; i < count; i++) {
		int events = m_events[i].events;
		if (events & EPOLLIN) {
			m_evLoop->Add
		}
	}
}
EpollDispatcher::~EpollDispatcher() {
	close(m_epfd);
	delete[] m_events;
}


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