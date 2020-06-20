#pragma once
#include "context.hpp"
#include "task_description.hpp"
#include "task.hpp"

class TaskFactory
{
public:
	
	template<typename Callable, typename ... Args>
	[[nodiscard]] auto createTask(Callable&& callable, Args&&... args)
	{
		auto taskDescriptionPointer = m_taskDescriptionPool.createTaskDescription(m_taskContextPool, m_taskGroupPool, std::forward<Callable>(callable), std::forward<Args>(args)...);
		return Task<std::invoke_result_t<Callable, Args...>>(taskDescriptionPointer);
	}

private:
	ContextPool m_taskContextPool;
	TaskGroupPool m_taskGroupPool;
	TaskDescriptionPool m_taskDescriptionPool;
};
