#include "task_group_queue.hpp"

#include "task_group.hpp"

void TaskGroupQueue::push(TaskGroup* group)
{
	group->topological();

	std::lock_guard guard(m_queueMutex);
	m_queue.emplace(group);
}

bool TaskGroupQueue::execute()
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

size_t TaskGroupQueue::getSize() const
{
	std::lock_guard guard(m_queueMutex);
	return m_queue.size();
}
