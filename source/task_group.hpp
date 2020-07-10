#pragma once
#include <optional>
#include <vector>
#include <iostream>
#include <mutex>
#include <cassert>
#include <atomic>

#include "private/handle_array.hpp"
#include "private/job_creator.hpp"

#include "context.hpp"
#include "task_node.hpp"

class TaskGroupPool;

class TaskGroup
{

public:
	using NodeType = TaskNode<Context, size_t>;

	struct TopologicalRound
	{
		std::vector< NodeType* > taskRound;
		std::uint32_t currentTask = std::numeric_limits<std::uint32_t>::max();
	};

	TaskGroup(std::uint16_t id, TaskGroupPool& pool) : m_groupId{id}, m_pool{ pool }
	{}

	~TaskGroup() = default;

	template<typename Callable, typename ... Args>
	size_t addNode(Callable&& callable, Args&&... args)
	{
		using DataType = Detail::PacketTask<Callable, Args ...>;
		auto job = Detail::JobCreator<DataType>::createJob();
		auto data = std::make_shared<DataType>(std::forward<Callable>(callable), std::forward<Args>(args)...);

		std::lock_guard guard(m_nodeMutex);
		size_t idx = m_nodes.size();
		m_nodes.emplace_back(*this, idx, std::move(job), std::move(data), std::any());
		++m_unfinishedJobNumbers;

		return idx;
	}

	void link(size_t from, size_t to);

	NodeType* getTaskNode(size_t nodeId);
	NodeType* getAvailableTask();

	void hasComplited(NodeType& node);

	bool isFinished() const;

	bool isLastTask() const;

	void topological();

	[[nodiscard]]
	Context& get(size_t index);

	void decreaseReferenceCount();
	void increaseReferenceCount();

private:

	struct TopologicalNode
	{
		TopologicalNode(size_t id, size_t count) : nodeId(id), parentCount(count)
		{}

		size_t nodeId;
		size_t parentCount = 0;
	};

	std::vector<TopologicalNode> getOrphanNode(std::vector<TopologicalNode>& nodes);

	void decreaseParentCount(std::vector<TopologicalNode>& vertexes, const std::vector<size_t>& adjances);
	void removeTaskGroup();

private:
	//temporary
	std::mutex m_nodeMutex;
	std::deque<NodeType> m_nodes;
	std::vector<TopologicalRound> m_topological;
	std::atomic<std::uint32_t> m_currentRound = 0;
	std::atomic<std::uint32_t> m_refCount = 1;
	std::atomic<std::uint32_t> m_unfinishedJobNumbers = 0;

	std::uint16_t m_groupId;
	TaskGroupPool& m_pool;
};

template<>
struct HandleArrayItemTraits<TaskGroup>
{
	static constexpr bool isStoredId = true;
};

class TaskGroupPool
{
public:
	using TaskGroupID = std::uint16_t;

public:
	TaskGroupID createTaskGroup()
	{
		return m_pool.emplace(*this);
	}

	TaskGroup& get(TaskGroupID taskGroupHandle)
	{
		return m_pool.get(taskGroupHandle);
	}

	void removeTaskGroup(TaskGroupID taskGroupHandle)
	{
		m_pool.free(taskGroupHandle);
	}

private:
	HandleArray<TaskGroup, std::uint16_t, 250> m_pool = {};
};
