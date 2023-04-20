#pragma once
#include <thread>
#include "EventLoop.h"
#include <condition_variable>

//定义子线程对应的结构体
class WorkerThread {
public:
	WorkerThread(int index);
	~WorkerThread();
	//启动线程
	void Run();
	EventLoop* getEvebtLoop() {
		return m_evLoop;
	}
private:
	//子线程的回调函数
	void Running();
	thread* m_thread;
	thread::id m_threadId;
	string m_name;
	mutex m_mutex;
	condition_variable m_cond;
	EventLoop* m_evLoop;
};
