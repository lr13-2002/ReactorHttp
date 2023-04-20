#include "EpollDispatcher.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "Log.h"

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
	int events = 0;
	debug("??? 添加的文件描述符 : %d", m_channel->get_Socket());
	if (m_channel->get_Event() & (int)FDEvent::ReadEvent) {
		events |= EPOLLIN;
	}
	if (m_channel->get_Event() & (int)FDEvent::WriteEvent) {
		events |= EPOLLOUT;
	}
	ev.events = events;
	return epoll_ctl(m_epfd, op, m_channel->get_Socket(), &ev);
}
int EpollDispatcher::add() {
	int ret = epollctl(EPOLL_CTL_ADD);
	if (ret == -1) {
		perror("epoll_ctl_add");
	}
	return ret;
}
int EpollDispatcher::remove() {
	int ret = epollctl(EPOLL_CTL_DEL);
	if (ret == -1) {
		perror("epoll_ctl_del");
	}
	m_channel->destroyCallback(const_cast<void*>(m_channel->get_Arg()));
	return ret;
}
int EpollDispatcher::modify() {
	int ret = epollctl(EPOLL_CTL_MOD);
	if (ret == -1) {
		perror("epoll_ctl_mod");
	}
	return ret;
}
int EpollDispatcher::dispatch(int timeout) {
	int count = epoll_wait(m_epfd, m_events, MaxNode, timeout*1000);
	debug("count : %d\n", count);
	for (int i = 0; i < count; i++) {
		int events = m_events[i].events;
		int fd = m_events[i].data.fd;
		if (events & EPOLLERR || events & EPOLLHUP) {
			continue;
		}
		if (events & EPOLLIN) {
			debug("epoll获取的文件描述符 : %d", fd);
			m_evLoop->eventActivate(fd, (int)FDEvent::ReadEvent);
		}
		if (events & EPOLLOUT) {
			m_evLoop->eventActivate(fd, (int)FDEvent::WriteEvent);
		}
	}
}
EpollDispatcher::~EpollDispatcher() {
	close(m_epfd);
	delete[] m_events;
}
