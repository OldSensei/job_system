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
		//TODO: handle auto deleter in case of FUBAR
		auto taskGroupHandle = m_taskGroupPool.createTaskGroup();
		auto& tg = m_taskGroupPool.get(taskGroupHandle);

		auto nodeId = tg.addNode(std::forward<Callable>(callable), std::forward<Args>(args)...);
		auto* taskNode = tg.getTaskNode(nodeId);

		return Task<std::invoke_result_t<Callable, Args...>>(taskNode);
	}

private:
	TaskGroupPool<Detail::POOL_SIZE> m_taskGroupPool = {};
};
