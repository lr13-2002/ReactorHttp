#include "WorkerThread.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include "Log.h"
WorkerThread::WorkerThread(int index) {
	m_evLoop = nullptr;
	m_thread = nullptr;
	m_threadId = this_thread::get_id();
	m_name = "SubThread-" + to_string(index);
}

WorkerThread::~WorkerThread() {
	if(m_thread != nullptr)
	delete m_thread;
}

void WorkerThread::Running() {
	m_mutex.lock();
	m_evLoop = new EventLoop(m_name);
	m_mutex.unlock();
	m_cond.notify_one();
	debug(m_name.data(), "Runing...");
	m_evLoop->Run();
	debug(m_name.data(), "Run!!!");
}

void WorkerThread::Run() {
	m_thread = new thread(&WorkerThread::Running, this);
	//阻塞一会等待创建完毕
	
	unique_lock<mutex>locker(m_mutex);
	while (m_evLoop == NULL) {
		m_cond.wait(locker);
	}
}
