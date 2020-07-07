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

class TaskGroup;

template<class ValueType, class IndexType>
class TaskNode
{
public:
	template<class Value = ValueType>
	TaskNode(TaskGroup& tg, IndexType id, Value&& value) : m_value(std::forward<Value>(value)), m_ID(id), m_group(&tg), m_unfinishedParentTasks(0)
	{}

	template<typename ... Args>
	TaskNode(TaskGroup& tg, IndexType id, Args&& ... args) :
		m_value{std::forward<Args>(args)...},
		m_ID(id),
		m_group(&tg),
		m_unfinishedParentTasks(0)
	{}

	TaskNode(TaskNode&& value) noexcept :
		m_adjanced(std::move(value.m_adjanced)),
		m_parents(std::move(value.m_parents)),
		m_value(std::move(value.m_value)),
		m_group(std::move(value.m_group)),
		m_ID(value.m_ID),
		m_unfinishedParentTasks(value.m_unfinishedParentTasks.load())
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

	ValueType& getValue()
	{
		return m_value;
	}

	TaskGroup& getGroup()
	{
		return *m_group;
	}

	const ValueType& getValue() const
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

	IndexType getID()
	{
		return m_ID;
	}

private:
	IndexType m_ID;
	ValueType m_value;
	TaskGroup* m_group;
	std::vector<IndexType> m_adjanced;
	std::vector<IndexType> m_parents;
	std::atomic<std::uint32_t> m_unfinishedParentTasks;
};

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

	void link(size_t from, size_t to)
	{
		auto& fromNode = m_nodes[from];
		fromNode.addAdjancedNode(to);
		auto& toNode = m_nodes[to];
		toNode.addParent(from);
	}

	NodeType* getTaskNode(size_t nodeId)
	{
		assert(nodeId < m_nodes.size());
		return &m_nodes[nodeId];
	}

	NodeType* getAvailableTask()
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

	void hasComplited(NodeType& node)
	{
		for(const auto& index : node.getAdjancedNodes() )
		{
			m_nodes[index].onParentTaskFinished();
		}

		if (--m_unfinishedJobNumbers == 0)
		{
			decreaseReferenceCount();
		}
	}

	bool isFinished() const
	{
		return m_unfinishedJobNumbers == 0;
	}

	bool isLastTask() const
	{
		return m_unfinishedJobNumbers == 1;
	}

	void topological()
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
	}

	Context& get(size_t index)
	{
		return m_nodes[index].getValue();
	}

	void decreaseReferenceCount()
	{
		assert(m_refCount);
		--m_refCount;

		//first approach: remove immediately 
		if (m_refCount == 0)
		{
			removeTaskGroup();
		}
	}

	void increaseReferenceCount()
	{
		++m_refCount;
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

	void removeTaskGroup();

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
struct IsStoredID<TaskGroup>
{
	static constexpr bool value = true;
};

using TaskGroupID = std::uint16_t;

template<std::uint32_t PoolSize>
struct TaskGroupData
{
	HandleArray<TaskGroup, std::uint16_t, PoolSize> m_pool;
};


class TaskGroupPool : private TaskGroupData<250>
{
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
};

inline void TaskGroup::removeTaskGroup()
{
	m_pool.removeTaskGroup(m_groupId);
}