#pragma once
#include<functional>
using namespace std;
class Channel {
	using handleFunc = function<int(void*)>;
public:
	Channel(int _fd, int _events, handleFunc _readFunc, handleFunc _writeFunc, handleFunc _destroyFunc, void* _arg):
		fd(_fd), events(_events), readCallback(_readFunc), writeCallback(_writeFunc), destroyCallback(_destroyFunc), arg(_arg){}

	//回调函数
	//修改 fd 的写事件
	void writeEventEnable(bool flag);
	//判断是否需要检测文件描述符的写事件
	bool isWriteEventEnable();
	handleFunc readCallback;
	handleFunc writeCallback;
	handleFunc destroyCallback;
	inline int get_Socket() {
		return fd;
	}
	inline int get_Event() {
		return events;
	}
	inline const void* get_Arg() {
		return arg;
	}
private:
	//文件描述符
	int fd;
	//事件
	int events;
	//回调函数的参数
	void* arg;
};
enum class FDEvent {
	TimeOur = 0x01,
	ReadEvent = 0x02,
	WriteEvent = 0x04
};