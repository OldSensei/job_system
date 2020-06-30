#pragma once

#include "config.hpp"
#include "context.hpp"
#include "task_group.hpp"


template<std::uint32_t>
class TaskDescriptionPool;

class TaskDescription
{
public:
	TaskDescription(TaskGroupPool<Detail::POOL_SIZE>& taskGroupPool, TaskGroupID taskgroup, size_t nodeId, TaskDescriptionPool<Detail::POOL_SIZE>& descriptionPool) :
		m_taskGroupID(taskgroup),
		m_taskNodeId(nodeId),
		m_taskDescriptionPool(descriptionPool),
		m_taskGroupPool(taskGroupPool)
	{}

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
		auto& taskgroup = m_taskGroupPool.get(m_taskGroupID);
		return taskgroup.get(m_taskNodeId).returnedValue;
	}

	void increaseReferenceCount() noexcept
	{
		auto& taskgroup = m_taskGroupPool.get(m_taskGroupID);
		taskgroup.increaseReferenceCount();
	}

	void decreaseReferenceCount()
	{
		auto& taskgroup = m_taskGroupPool.get(m_taskGroupID);
		taskgroup.decreaseReferenceCount();
	}

private:
	TaskGroupID m_taskGroupID;
	ContextID m_contextID;
	size_t m_taskNodeId;

	TaskDescriptionPool<Detail::POOL_SIZE>& m_taskDescriptionPool;
	TaskGroupPool<Detail::POOL_SIZE>& m_taskGroupPool;
};

template<std::uint32_t PoolSize>
class TaskDescriptionPool
{
public:
	template<typename Callable, typename ... Args>
	TaskDescription* createTaskDescription(TaskGroupPool<PoolSize>& tgPool, Callable&& callable, Args&&... args)
	{
		//TODO: handle auto deleter in case of FUBAR
		auto taskGroupHandle = tgPool.createTaskGroup();

		auto& tg = tgPool.get(taskGroupHandle);
		auto nodeId = tg.addNode(std::forward<Callable>(callable), std::forward<Args>(args)...);

		auto taskDescriptionHandle = m_pool.emplace(tgPool, taskGroupHandle, nodeId, *this );
		return &m_pool.get(taskDescriptionHandle);
	}

	template<typename Callable, typename ... Args>
	TaskDescription* createLinkedTaskDescription(TaskGroupPool<PoolSize>& tgPool, const TaskGroupID& groupId, size_t parentNodeId, Callable&& callable, Args&&... args)
	{
		//TODO: handle auto deleter in case of FUBAR
		auto& tg = tgPool.get(groupId);
		auto nodeId = tg.addNode(std::forward<Callable&&>(callable), std::forward<Args>(args)...);
		tg.link(parentNodeId, nodeId);

		auto taskDescriptionHandle = m_pool.emplace(tgPool, groupId, nodeId, *this);
		return &m_pool.get(taskDescriptionHandle);
	}

private:
	HandleArray<TaskDescription, std::uint16_t, PoolSize > m_pool;
};