#pragma once
#include "config.hpp"
#include "context.hpp"
#include "task_description.hpp"
#include "task.hpp"

class TaskFactory
{
public:
	
	template<typename Callable, typename ... Args>
	[[nodiscard]]
	auto createTask(Callable&& callable, Args&&... args)
	{
		auto taskDescriptionPointer = m_taskDescriptionPool.createTaskDescription(m_taskGroupPool, std::forward<Callable>(callable), std::forward<Args>(args)...);
		return Task<std::invoke_result_t<Callable, Args...>>(taskDescriptionPointer);
	}

private:
	TaskGroupPool<Detail::POOL_SIZE> m_taskGroupPool = {};
	TaskDescriptionPool<Detail::POOL_SIZE> m_taskDescriptionPool = {};
};
