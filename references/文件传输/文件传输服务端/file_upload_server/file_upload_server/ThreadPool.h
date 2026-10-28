#pragma once
#include <vector>
#include <queue>
#include <pthread.h>
#include <iostream>
#include "BusinessTask.h"

using namespace std;

class ThreadPool
{
public:
	explicit ThreadPool(size_t threadCount);
	~ThreadPool();
	void submit(BusinessTask* task);
	size_t queueSize();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
private:
	static void* workerThread(void* arg);

	size_t m_threadCount;
	vector<pthread_t> m_workers;
	queue<BusinessTask*> m_tasks;
	pthread_mutex_t m_queueMutex;
	pthread_cond_t m_condition;
	bool m_stop;
};