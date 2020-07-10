#pragma once

#include "task_executor.hpp"
#include "utils/event.hpp"

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
		m_value{ std::forward<Args>(args)... },
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

	void wait(TaskExecuter& executor)
	{
		executor.push(*m_group);
		m_finishedEvent.wait();
	}

	void fireOnFinishedEvent()
	{
		m_finishedEvent.notify();
	}

private:
	IndexType m_ID;
	ValueType m_value;
	TaskGroup* m_group;

	std::vector<IndexType> m_adjanced;
	std::vector<IndexType> m_parents;
	std::atomic<std::uint32_t> m_unfinishedParentTasks;

	Event m_finishedEvent;
};

