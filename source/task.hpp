#pragma once
#include <tuple>
#include <memory>
#include <atomic>
#include <any>
#include <vector>
#include <forward_list>
#include <list>

#include "task_group.hpp"

template<typename ReturnedType>
class Task
{
public:

	Task( TaskGroup::NodeType* taskNode ) noexcept :
		m_taskNode(taskNode)
	{
		assert(m_taskNode);
		m_taskNode->getGroup().increaseReferenceCount();
	}

	Task(const Task& other) noexcept :
		m_taskNode(other.m_taskNode)
	{
		m_taskNode->getGroup().increaseReferenceCount();
	}

	Task(Task&& other) noexcept :
		m_taskNode(other.m_taskNode)
	{
		other.m_taskNode = nullptr;
	}

	~Task()
	{
		if (m_taskNode)
		{
			m_taskNode->getGroup().decreaseReferenceCount();
		}
	}

	Task& operator=(Task& other) noexcept
	{
		if (&other != this)
		{
			if (m_taskNode)
			{
				m_taskNode->getGroup().decreaseReferenceCount();
			}

			m_taskNode = other.m_description;
			m_taskNode->getGroup().increaseReferenceCount();
		}
	}

	template<typename Callable, typename ... Args>
	auto then(Callable&& callable, Args&&... args)
	{
		auto& group = m_taskNode->getGroup();

		auto nodeID = group.addNode(std::forward<Callable>(callable), *this, std::forward<Args>(args)...);
		group.link(m_taskNode->getID(),nodeID);

		auto* taskNode = group.getTaskNode(nodeID);
		return Task< std::invoke_result_t<Callable, std::add_lvalue_reference_t< std::remove_pointer_t< decltype(this) >, Args...> > >(taskNode);
	}

	template<typename Type_ = ReturnedType>
	[[nodiscard]]
	typename std::enable_if_t< !std::is_same<Type_, void>::value, Type_ > get()
	{
		assert(m_taskNode);
		return std::any_cast<ReturnedType>(m_taskNode->getReturnedValue());
	}

	template<typename Type_ = ReturnedType>
	typename std::enable_if_t< std::is_same<Type_, void>::value > get()
	{}

private:
	friend class TaskExecuter;

	TaskGroup::NodeType* m_taskNode;
};
