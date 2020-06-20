#pragma once

#include "context.hpp"
#include "task_group.hpp"


class TaskDescriptionPool;

class TaskDescription
{
public:
	TaskDescription(ContextPool& contextPool, ContextID context, TaskGroupPool& taskGroupPool, TaskGroupID taskgroup, size_t nodeId, TaskDescriptionPool& descriptionPool) :
		m_taskGroupID(taskgroup),
		m_contextID(context),
		m_taskNodeId(nodeId),
		m_contextPool(contextPool),
		m_taskDescriptionPool(descriptionPool),
		m_taskGroupPool(taskGroupPool)
	{}

	[[nodiscard]]
	ContextPool& getContextPool() noexcept { return m_contextPool; }

	[[nodiscard]]
	TaskDescriptionPool& getTaskDescriptionPool() noexcept { return m_taskDescriptionPool; }

	[[nodiscard]]
	TaskGroupPool& getTaskGroupPool() noexcept { return m_taskGroupPool; }

	[[nodiscard]]
	TaskGroupID getTaskGroupID() const noexcept { return m_taskGroupID; } 

	[[nodiscard]]
	size_t getNodeID() const noexcept { return m_taskNodeId; }

	[[nodiscard]]
	TaskGroup* getTaskGroup() noexcept { return &m_taskGroupPool.get(m_taskGroupID); }

	[[nodiscard]]
	std::any getReturnedValue()
	{
		return m_contextPool.get(m_contextID).returnedValue;
	}

private:
	TaskGroupID m_taskGroupID;
	ContextID m_contextID;
	size_t m_taskNodeId;

	ContextPool& m_contextPool;
	TaskDescriptionPool& m_taskDescriptionPool;
	TaskGroupPool& m_taskGroupPool;
};

class TaskDescriptionPool
{
public:
	template<typename Callable, typename ... Args>
	TaskDescription* createTaskDescription(ContextPool& ctxPool, TaskGroupPool& tgPool, Callable&& callable, Args&&... args)
	{
		//TODO: handle auto deleter in case of FUBAR
		auto contextHandle = ctxPool.createContext(std::forward<Callable>(callable), std::forward<Args>(args)...);
		auto& ctx = ctxPool.get(contextHandle);
		auto taskGroupHandle = tgPool.createTaskGroup();

		auto& tg = tgPool.get(taskGroupHandle);
		auto nodeId = tg.addNode(&ctx);

		auto taskDescriptionHandle = m_pool.emplace(ctxPool, contextHandle, tgPool, taskGroupHandle, nodeId, *this );
		return &m_pool.get(taskDescriptionHandle);
	}

	template<typename Callable, typename ... Args>
	TaskDescription* createLinkedTaskDescription(ContextPool& ctxPool, TaskGroupPool& tgPool, const TaskGroupID& groupId, size_t parentNodeId, Callable&& callable, Args&&... args)
	{
		//TODO: handle auto deleter in case of FUBAR
		auto contextHandle = ctxPool.createContext(std::forward<Callable&&>(callable), std::forward<Args>(args)...);
		auto& ctx = ctxPool.get(contextHandle);

		auto& tg = tgPool.get(groupId);
		auto nodeId = tg.addNode(&ctx);
		tg.link(parentNodeId, nodeId);

		auto taskDescriptionHandle = m_pool.emplace(ctxPool, contextHandle, tgPool, groupId, nodeId, *this);
		return &m_pool.get(taskDescriptionHandle);

		//auto handle = JobSystem::g_cp.createContext(std::forward<Callable&&>(callable), *this, std::forward<Args>(args)...);
		//auto nodeId = m_taskGroup->addNode(handle);
		//m_taskGroup->link( getTaskNodeId(), nodeId );
	}

private:
	//TODO: Remove HARDCODE
	HandleArray<TaskDescription, std::uint16_t, 20 > m_pool;
};