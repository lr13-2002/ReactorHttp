#include "channel.h"
#include <stdlib.h>
#include "Log.h"
void Channel::writeEventEnable(bool flag) {
	if (flag) {
		this->events |= (int)FDEvent::WriteEvent;
	}
	else {
		this->events &= (~(int)FDEvent::WriteEvent);
	}
}

bool Channel::isWriteEventEnable() {

	return this->events & (int)FDEvent::WriteEvent;
}
