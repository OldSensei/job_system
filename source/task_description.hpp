#pragma once

#include "config.hpp"
#include "context.hpp"
#include "task_group.hpp"


template<std::uint32_t>
class TaskDescriptionPool;

class TaskDescription
{
public:
	TaskDescription(ContextPool<Detail::POOL_SIZE>& contextPool, ContextID context, TaskGroupPool<Detail::POOL_SIZE>& taskGroupPool, TaskGroupID taskgroup, size_t nodeId, TaskDescriptionPool<Detail::POOL_SIZE>& descriptionPool) :
		m_taskGroupID(taskgroup),
		m_contextID(context),
		m_taskNodeId(nodeId),
		m_contextPool(contextPool),
		m_taskDescriptionPool(descriptionPool),
		m_taskGroupPool(taskGroupPool)
	{}

	[[nodiscard]]
	auto& getContextPool() noexcept { return m_contextPool; }

	[[nodiscard]]
	auto& getTaskDescriptionPool() noexcept { return m_taskDescriptionPool; }

	[[nodiscard]]
	auto& getTaskGroupPool() noexcept { return m_taskGroupPool; }

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

	void increaseReferenceCount() noexcept
	{
		auto& taskgroup = m_taskGroupPool.get(m_taskGroupID);
		taskgroup.increaseReferenceCount(m_taskNodeId);
	}

	void decreaseReferenceCount()
	{
		auto& taskgroup = m_taskGroupPool.get(m_taskGroupID);
		taskgroup.decreaseReferenceCount(m_taskNodeId);
	}

private:
	TaskGroupID m_taskGroupID;
	ContextID m_contextID;
	size_t m_taskNodeId;

	ContextPool<Detail::POOL_SIZE>& m_contextPool;
	TaskDescriptionPool<Detail::POOL_SIZE>& m_taskDescriptionPool;
	TaskGroupPool<Detail::POOL_SIZE>& m_taskGroupPool;
};

template<std::uint32_t PoolSize>
class TaskDescriptionPool
{
public:
	template<typename Callable, typename ... Args>
	TaskDescription* createTaskDescription(ContextPool<PoolSize>& ctxPool, TaskGroupPool<PoolSize>& tgPool, Callable&& callable, Args&&... args)
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
	TaskDescription* createLinkedTaskDescription(ContextPool<PoolSize>& ctxPool, TaskGroupPool<PoolSize>& tgPool, const TaskGroupID& groupId, size_t parentNodeId, Callable&& callable, Args&&... args)
	{
		//TODO: handle auto deleter in case of FUBAR
		auto contextHandle = ctxPool.createContext(std::forward<Callable&&>(callable), std::forward<Args>(args)...);
		auto& ctx = ctxPool.get(contextHandle);

		auto& tg = tgPool.get(groupId);
		auto nodeId = tg.addNode(&ctx);
		tg.link(parentNodeId, nodeId);

		auto taskDescriptionHandle = m_pool.emplace(ctxPool, contextHandle, tgPool, groupId, nodeId, *this);
		return &m_pool.get(taskDescriptionHandle);
	}

private:
	HandleArray<TaskDescription, std::uint16_t, PoolSize > m_pool;
};