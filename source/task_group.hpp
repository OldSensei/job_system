#pragma once
#include <optional>
#include <vector>
#include <iostream>
#include <mutex>
#include <cassert>
#include <atomic>

#include "private/handle_array.hpp"
#include "context.hpp"

template<class ValueType, class IndexType>
class TaskNode
{
public:
	template<class Value = ValueType>
	TaskNode(Value&& value) : m_value(std::forward<Value>(value)), m_unfinishedParentTasks(0), m_referenceCount(1)
	{}

	TaskNode(TaskNode&& value) noexcept :
		m_adjanced(std::move(value.m_adjanced)),
		m_parents(std::move(value.m_parents)),
		m_value(std::move(value.m_value)),
		m_unfinishedParentTasks(value.m_unfinishedParentTasks.load()),
		m_referenceCount(value.m_referenceCount.load())
	{}

	void addAdjancedNode(const IndexType& nodeIndex)
	{
		m_adjanced.emplace_back(nodeIndex);
	}

	void addParent(const IndexType& nodeIndex)
	{
		m_parents.emplace_back(nodeIndex);
		++this->m_unfinishedParentTasks;
	}

	const std::vector<IndexType>& getAdjancedNodes() const
	{
		return m_adjanced;
	}

	const std::vector<IndexType>& getParentsNodes() const
	{
		return m_parents;
	}

	ValueType getValue() const
	{
		return m_value;
	}

	size_t getParentsCount() const
	{
		return m_parents.size();
	}

	bool isAvailable() const
	{
		return m_unfinishedParentTasks == 0;
	}

	void onParentTaskFinished()
	{
		assert(m_unfinishedParentTasks > 0);
		--m_unfinishedParentTasks;
	}

	void decreaseReferenceCount()
	{
		assert( m_referenceCount > 0 );
		--m_referenceCount;
	}

	void increaseReferenceCount()
	{
		++m_referenceCount;
	}

private:
	ValueType m_value;
	std::vector<IndexType> m_adjanced;
	std::vector<IndexType> m_parents;
	std::atomic<std::uint32_t> m_unfinishedParentTasks;
	std::atomic<std::uint64_t> m_referenceCount;
};

class TaskGroup
{

public:
	struct TopologicalRound
	{
		std::vector< TaskNode<Context*, size_t>* > taskRound;
		std::uint32_t currentTask = std::numeric_limits<std::uint32_t>::max();
	};

	size_t addNode(Context* value)
	{
		std::lock_guard guard(m_nodeMutex);
		m_nodes.emplace_back(value);
		m_nodes.back().increaseReferenceCount();

		size_t idx = m_nodes.size() - 1;
		return idx;
	}

	void link(size_t from, size_t to)
	{
		auto& fromNode = m_nodes[from];
		fromNode.addAdjancedNode(to);
		auto& toNode = m_nodes[to];
		toNode.addParent(from);
	}

	TaskNode<Context*, size_t>* getAvailableTask()
	{
		if (m_currentRound >= m_topological.size())
		{
			return nullptr;
		}

		if (m_topological[m_currentRound].currentTask >= m_topological[m_currentRound].taskRound.size())
		{
			++m_currentRound;
			if (m_currentRound == m_topological.size())
			{
				return nullptr;
			}
		}

		auto& round = m_topological[m_currentRound];
		std::uint32_t index = round.currentTask;
		while (index < round.taskRound.size()) // TODO: avoid going through all tasks
		{
			if (round.taskRound[index]->isAvailable())
			{
				auto copy = round.taskRound[round.currentTask];
				round.taskRound[round.currentTask] = round.taskRound[index];
				round.taskRound[index] = copy;
				return round.taskRound[round.currentTask++];
			}
			++index;
		}

		return nullptr;
	}

	void hasComplited(TaskNode<Context*, size_t>& task)
	{
		for(const auto& index : task.getAdjancedNodes() )
		{
			m_nodes[index].onParentTaskFinished();
		}

		const auto& parents = task.getParentsNodes();
		for (const auto& parentIndex : parents)
		{
			m_nodes[parentIndex].decreaseReferenceCount();
		}

	}

	bool isFinished() const
	{
		return m_currentRound >= m_topological.size();
	}

	std::vector<TopologicalRound> topological()
	{
		std::vector<TopologicalRound> result;

		std::vector<TopologicalNode> vertexes;
		size_t idx = 0;
		for (const auto& node : m_nodes)
		{
			vertexes.emplace_back( idx, node.getParentsCount() );
			++idx;
		}

		while (!vertexes.empty())
		{
			TopologicalRound round;
			auto S = getOrphanNode(vertexes);
			for ( auto& e : S )
			{
				auto& node = m_nodes[e.nodeId];
				const auto& adjances = node.getAdjancedNodes();
				round.taskRound.push_back( &node );
				decreaseParentCount(vertexes, adjances);
			}
			round.currentTask = 0;
			result.emplace_back(round);
		}


		m_topological = result;
		return result;
	}

	Context* get(size_t index) const
	{
		return m_nodes[index].getValue();
	}

	inline void decreaseReferenceCount(size_t nodeID)
	{
		m_nodes[nodeID].decreaseReferenceCount();
	}

	inline void increaseReferenceCount(size_t nodeID)
	{
		m_nodes[nodeID].increaseReferenceCount();
	}

private:

	struct TopologicalNode
	{
		TopologicalNode(size_t id, size_t count) : nodeId(id), parentCount(count)
		{}

		size_t nodeId;
		size_t parentCount = 0;
	};

	std::vector<TopologicalNode> getOrphanNode(std::vector<TopologicalNode>& nodes)
	{
		std::vector<TopologicalNode> result;

		auto iter = nodes.begin();
		while ( iter != nodes.end() )
		{
			if (iter->parentCount == 0)
			{
				result.push_back(*iter);
				iter = nodes.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		return result;
	}

	void decreaseParentCount(std::vector<TopologicalNode>& vertexes, const std::vector<size_t>& adjances)
	{
		for (auto& vertex : vertexes)
		{
			const auto& it = std::find_if(adjances.begin(), adjances.end(), [id = vertex.nodeId](const size_t& node)
			{
				return id == node;
			});

			if (it != adjances.end())
			{
				--vertex.parentCount;
			}
		}
	}

	//temporary
	std::mutex m_nodeMutex;
	std::vector<TaskNode<Context*, size_t>> m_nodes;
	std::vector<TopologicalRound> m_topological;
	std::atomic<std::uint32_t> m_currentRound = 0;
};


using TaskGroupID = std::uint16_t;

template<std::uint32_t PoolSize>
class TaskGroupPool
{
public:
	TaskGroupID createTaskGroup()
	{
		return m_pool.emplace();
	}

	TaskGroup& get(TaskGroupID taskGroupHandle)
	{
		return m_pool.get(taskGroupHandle);
	}

private:
	HandleArray<TaskGroup, std::uint16_t, PoolSize> m_pool;
};
