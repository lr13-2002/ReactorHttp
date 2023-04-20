#pragma once
#include "Dispatcher.h"
#include <sys/epoll.h>

class EpollDispatcher : public Dispatcher {
public:
	EpollDispatcher(EventLoop* evLoop);
	~EpollDispatcher();
	int add() override;
	int remove() override;
	int modify() override;
	int dispatch(int timeout = 2) override;
private:
	int epollctl(int op);
	int m_epfd;
	struct epoll_event* m_events;
	const int MaxNode = 520;
};