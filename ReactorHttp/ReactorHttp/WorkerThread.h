#pragma once
#include <pthread.h>
#include "EventLoop.h"

//�������̶߳�Ӧ�Ľṹ��
struct WorkerThread {
	pthread_t threadID;
	char name[24];
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	struct EventLoop* evLoop;
};
//��ʼ��
int workerThreadInit(struct WorkerThread* thread, int index);
//���̵߳Ļص�����
void* subThreadRunning(void* arg);
//�����߳�
void workerThreadRun(struct WorkerThread* thread);
