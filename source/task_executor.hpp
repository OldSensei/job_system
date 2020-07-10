#pragma once
#include <queue>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>

#include "task_group_queue.hpp"
#include "utils/semaphore.hpp"

class TaskExecuter
{
public:
	TaskExecuter(std::uint16_t threadCount);
	~TaskExecuter();
	void push(TaskGroup& group);
	void wait();

private:
	void work();
	void initializeWorkers();

private:
	std::vector<std::thread> m_workers;
	TaskGroupQueue m_highPriorityQueue;

	const std::uint16_t m_threadCount;

	Semaphore m_semaphore;
	volatile bool m_isEnabled;
};
