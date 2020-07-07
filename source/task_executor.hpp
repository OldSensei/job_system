#pragma once
#include <queue>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>

#include "task.hpp"
#include "task_group.hpp"

class Semaphore
{
public:
	Semaphore( std::uint32_t count = 1) :
		MAX_COUNT(count),
		m_count(MAX_COUNT)
	{
	};

	Semaphore(const Semaphore&) = delete;

	Semaphore(Semaphore&& other) noexcept :
		m_count(other.m_count),
		MAX_COUNT(other.MAX_COUNT)
	{

	}

	void wait() noexcept
	{
		std::unique_lock lck(m_mutex);
		m_cv.wait(lck, [this]() { return this->m_count > 0; });
		--m_count;
	}

	void notify() noexcept
	{
		std::unique_lock lck(m_mutex);
		++m_count;
		m_cv.notify_one();
	}

	void notifyAll() noexcept
	{
		std::unique_lock lck(m_mutex);
		m_count = MAX_COUNT;
		m_cv.notify_all();
	}

private:
	std::mutex m_mutex;
	std::condition_variable m_cv;
	const std::uint32_t MAX_COUNT;
	volatile std::uint32_t m_count;
};

class TaskGroupQueue
{
public:
	template<class TaskGroupPointer>
	void push(TaskGroupPointer&& group)
	{
		std::lock_guard guard(m_queueMutex);
		m_queue.emplace( std::forward<TaskGroupPointer>(group) );
	}

	bool execute()
	{
		m_queueMutex.lock();

		for (auto index = 0; index < m_queue.size(); ++index)
		{
			auto& taskgroup = m_queue.front();

			auto task = taskgroup->getAvailableTask();

			if (task)
			{
				if (taskgroup->isLastTask())
				{
					m_queue.pop();
				}

				m_queueMutex.unlock();
				auto& ctx = task->getValue();
				ctx.job(ctx.data, ctx.returnedValue);

				taskgroup->hasComplited(*task);
				return true;
			}
			else
			{
				if (!taskgroup->isFinished())
				{
					m_queue.push(std::move(taskgroup));
				}
				m_queue.pop();
			}
		}
		m_queueMutex.unlock();
		return false;
	}

	size_t getSize()
	{
		std::lock_guard guard(m_queueMutex);
		return m_queue.size();
	}

private:
	std::queue<TaskGroup*> m_queue;
	std::mutex m_queueMutex;
};

class TaskExecuter
{
public:
	TaskExecuter(std::uint16_t threadCount) :
		m_semaphore(threadCount),
		m_threadCount(threadCount),
		m_isEnabled(true)
	{
		m_workers.reserve(threadCount);
		initializeWorkers();
	};

	~TaskExecuter()
	{
		m_isEnabled = false;
		m_semaphore.notifyAll();
		std::for_each(m_workers.begin(), m_workers.end(), [](auto& worker) { worker.join(); });
	}

	template<typename T>
	void push(Task<T>& t)
	{
		auto& group = t.m_taskNode->getGroup();
		group.topological();
		m_highPriorityQueue.push(&group);
		this->m_semaphore.notify();
	}

	void wait()
	{
		while( m_highPriorityQueue.getSize() != 0 )
		{
			std::this_thread::yield();
		}
	}
private:
	void work()
	{
		while (m_isEnabled)
		{
			this->m_semaphore.wait();

			if (m_isEnabled && m_highPriorityQueue.execute())
			{
				m_semaphore.notify();
			}
		}
	}

	void initializeWorkers()
	{
		for (std::uint16_t index = 0; index < m_threadCount; ++index)
		{
			m_workers.emplace_back( [this](){this->work();}	);
		}
	}



private:
	std::vector<std::thread> m_workers;
	TaskGroupQueue m_highPriorityQueue;

	const std::uint16_t m_threadCount;

	Semaphore m_semaphore;
	volatile bool m_isEnabled;
};
