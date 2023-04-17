#include "ThreadPool.h"
#include <assert.h>
#include <stdio.h>
struct ThreadPoll* threadPoolInit(struct EventLoop* mainLoop, int count) {
	struct ThreadPool* pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
	pool->index = 0;
	pool->isStart = 0;
	pool->mainLoop = mainLoop;
	pool->threadNum = count;
	pool->workerThreads = (struct WorkerThread*)malloc(sizeof(struct WorkerThread) * count);
	return pool;
}

void threadPoolRun(struct ThreadPool* pool) {
	assert(pool && !pool->isStart);
	if (pool->mainLoop->threadID != pthread_self()) {
		exit(0);
	}
	pool->isStart = true;
	if (pool->threadNum) {
		for (int i = 0; i < pool->threadNum; i++) {
			workerThreadInit(&pool->workerThreads[i], i);
			workerThreadRun(&pool->workerThreads[i]);
		}
	}
}

struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool) {
	assert(pool->isStart);
	if (pool->mainLoop->threadID != pthread_self()) {
		exit(0);
	}
	struct EventLoop* evLoop = pool->mainLoop;
	if (pool->threadNum > 0) {
		evLoop = pool->workerThreads[pool->index].evLoop;
		pool->index = ++pool->index % pool->threadNum;
	}
	return evLoop;
	
}
