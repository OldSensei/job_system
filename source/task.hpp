#pragma once
#include <tuple>
#include <memory>
#include <atomic>
#include <any>
#include <vector>
#include <forward_list>
#include <list>

#include "task_description.hpp"

template<typename ReturnedType>
class Task
{
public:

	Task( TaskDescription* description ) :
		m_description(description)
	{}

	template<typename Callable, typename ... Args>
	auto then(Callable&& callable, Args&&... args)
	{
		auto& contextPool = m_description->getContextPool();
		auto& descriptionPool = m_description->getTaskDescriptionPool();
		auto& taskGroupPool = m_description->getTaskGroupPool();
		auto taskGroupID = m_description->getTaskGroupID();
		auto taskNodeId = m_description->getNodeID();

		auto taskDescriptionPointer = descriptionPool.createLinkedTaskDescription(contextPool, taskGroupPool, taskGroupID, taskNodeId,
			std::forward<Callable>(callable),
			*this,
			std::forward<Args>(args)...);

		return Task< std::invoke_result_t<Callable, std::add_lvalue_reference_t< std::remove_pointer_t< decltype(this) >, Args...> > >(taskDescriptionPointer);
	}

	template<typename Type_ = ReturnedType>
	typename std::enable_if_t< !std::is_same<Type_, void>::value, Type_ > get()
	{
		return std::any_cast<ReturnedType>(m_description->getReturnedValue());
	}

	template<typename Type_ = ReturnedType>
	typename std::enable_if_t< std::is_same<Type_, void>::value > get()
	{}

private:
	friend class TaskExecuter;
	TaskDescription* m_description;
};
