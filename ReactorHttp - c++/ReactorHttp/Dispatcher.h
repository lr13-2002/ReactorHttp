#pragma once
#include "channel.h"
#include "EventLoop.h"
#include <string>
class EventLoop;
class Channel;
using namespace std;
class Dispatcher {
public:
	Dispatcher(EventLoop* evLoop);
	virtual ~Dispatcher();
	virtual int add();
	virtual int remove();
	virtual int modify();
	virtual int dispatch(int timeout = 2);
	inline void setChannel(Channel* channel)
	{
		m_channel = channel;
	}
protected:
	EventLoop* m_evLoop;
	Channel* m_channel;
	string m_name = string();
};