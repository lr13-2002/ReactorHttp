#include "channel.h"
#include <stdlib.h>
#include "Log.h"
int cnt = 0;
struct Channel* channelInit(int fd, int events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg) {
	cnt++;
	Debug("!!!!! %d", cnt);
	Debug("ChannelInit.... %u", sizeof(struct Channel));
	struct Channel* channel = (struct Channel*)malloc(sizeof(struct Channel));
	Debug("ChannelInit!!");
	channel->fd = fd;
	channel->events = events;
	channel->readCallback = readFunc;
	channel->writeCallback = writeFunc;
	channel->destroyCallback = writeFunc;
	channel->arg = arg;
	return channel;
}

void writeEventEnable(struct Channel* channel, bool flag) {
	if (flag) {
		channel->events |= WriteEvent;
	}
	else {
		channel->events &= (~WriteEvent);
	}
}

bool isWriteEventEnable(struct Channel* channel) {

	return channel->events & WriteEvent;
}
