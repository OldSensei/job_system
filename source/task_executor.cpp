#include "task_executor.hpp"

TaskExecuter::TaskExecuter(std::uint16_t threadCount) :
	m_semaphore(threadCount),
	m_threadCount(threadCount),
	m_isEnabled(true)
{
	m_workers.reserve(threadCount);
	initializeWorkers();
};

TaskExecuter::~TaskExecuter()
{
	m_isEnabled = false;
	m_semaphore.notifyAll();
	std::for_each(m_workers.begin(), m_workers.end(), [](auto& worker) { worker.join(); });
}

void TaskExecuter::push(TaskGroup& group)
{
	m_highPriorityQueue.push(&group);
	m_semaphore.notifyAll();
}

void TaskExecuter::wait()
{
	while (m_highPriorityQueue.getSize() != 0)
	{
		std::this_thread::yield();
	}
}

void TaskExecuter::work()
{
	while (m_isEnabled)
	{
		this->m_semaphore.wait();

		if (m_isEnabled && m_highPriorityQueue.execute())
		{
			m_semaphore.notifyAll();
		}
	}
}

void TaskExecuter::initializeWorkers()
{
	for (std::uint16_t index = 0; index < m_threadCount; ++index)
	{
		m_workers.emplace_back([this]() {this->work(); });
	}
}
