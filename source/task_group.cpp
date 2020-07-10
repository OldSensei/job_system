#include "task_group.hpp"

void TaskGroup::link(size_t from, size_t to)
{
	auto& fromNode = m_nodes[from];
	fromNode.addAdjancedNode(to);
	auto& toNode = m_nodes[to];
	toNode.addParent(from);
}

TaskGroup::NodeType* TaskGroup::getTaskNode(size_t nodeId)
{
	assert(nodeId < m_nodes.size());
	return &m_nodes[nodeId];
}

TaskGroup::NodeType* TaskGroup::getAvailableTask()
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

void TaskGroup::hasComplited(NodeType& node)
{
	for (const auto& index : node.getAdjancedNodes())
	{
		m_nodes[index].onParentTaskFinished();
	}

	node.fireOnFinishedEvent();

	if (--m_unfinishedJobNumbers == 0)
	{
		decreaseReferenceCount();
	}
}

bool TaskGroup::isFinished() const
{
	return m_unfinishedJobNumbers == 0;
}

bool TaskGroup::isLastTask() const
{
	return m_unfinishedJobNumbers == 1;
}

void TaskGroup::topological()
{
	std::vector<TopologicalRound> result;

	std::vector<TopologicalNode> vertexes;
	size_t idx = 0;
	for (const auto& node : m_nodes)
	{
		vertexes.emplace_back(idx, node.getParentsCount());
		++idx;
	}

	while (!vertexes.empty())
	{
		TopologicalRound round;
		auto S = getOrphanNode(vertexes);
		for (auto& e : S)
		{
			auto& node = m_nodes[e.nodeId];
			const auto& adjances = node.getAdjancedNodes();
			round.taskRound.push_back(&node);
			decreaseParentCount(vertexes, adjances);
		}
		round.currentTask = 0;
		result.emplace_back(round);
	}


	m_topological = result;
}

Context& TaskGroup::get(size_t index)
{
	return m_nodes[index].getValue();
}

void TaskGroup::decreaseReferenceCount()
{
	assert(m_refCount);
	--m_refCount;

	//first approach: remove immediately 
	if (m_refCount == 0)
	{
		removeTaskGroup();
	}
}

void TaskGroup::increaseReferenceCount()
{
	++m_refCount;
}

std::vector<TaskGroup::TopologicalNode> TaskGroup::getOrphanNode(std::vector<TopologicalNode>& nodes)
{
	std::vector<TopologicalNode> result;

	auto iter = nodes.begin();
	while (iter != nodes.end())
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

void TaskGroup::decreaseParentCount(std::vector<TopologicalNode>& vertexes, const std::vector<size_t>& adjances)
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

void TaskGroup::removeTaskGroup()
{
	m_pool.removeTaskGroup(m_groupId);
}