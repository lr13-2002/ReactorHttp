#pragma once
#include "Dispatcher.h"
#include <sys/epoll.h>

class EpollDispatcher : public Dispatcher {
public:
	EpollDispatcher(EventLoop* evLoop);
	~EpollDispatcher();
	int add();
	int remove();
	int modify();
	int dispatch(int timeout = 2);
private:
	int epollctl(int op);
	int m_epfd;
	epoll_event* m_events;
	const int MaxNode = 520;
};