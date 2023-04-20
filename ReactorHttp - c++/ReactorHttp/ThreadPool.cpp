#include "ThreadPool.h"
#include <assert.h>
#include <stdio.h>
ThreadPool::ThreadPool(EventLoop* mainLoop, int count) {
	m_index = 0;
	m_isStart = 0;
	m_mainLoop = mainLoop;
	m_threadNum = count;
	m_workerThreads.clear();
}
ThreadPool::~ThreadPool() {
	for (auto item : m_workerThreads) {
		delete item;
	}
}
void ThreadPool::Run() {
	assert(pool && !pool->isStart);
	if (m_mainLoop->getThreadID() != this_thread::get_id()) {
		exit(0);
	}
	m_isStart = true;
	if (m_threadNum) {
		for (int i = 0; i < m_threadNum; i++) {
			WorkerThread* subThread = new WorkerThread(i);
			subThread->Run();
			m_workerThreads.push_back(subThread);
		}
	}
}

EventLoop* ThreadPool::takeWorkerEventLoop() {
	assert(m_isStart);
	if (m_mainLoop->getThreadID() != this_thread::get_id()) {
		exit(0);
	}
	EventLoop* evLoop = m_mainLoop;
	if (m_threadNum > 0) {
		evLoop = m_workerThreads[m_index]->getEvebtLoop();
		m_index = ++m_index % m_threadNum;
	}
	return evLoop;
	
}
