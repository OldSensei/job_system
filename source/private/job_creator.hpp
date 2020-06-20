#pragma once
#include <memory>
#include <any>

namespace Detail
{
	template<typename PacketTaskType, typename ReturnedType>
	struct JobCreatorImpl
	{
		static auto createJob()
		{
			return [](std::shared_ptr<void>& data, std::any& returnedValue)
			{
				auto* work = static_cast<PacketTaskType*>(data.get());
				returnedValue = std::apply(work->callable, work->arguments);
			};
		}
	};

	template<typename PacketTaskType>
	struct JobCreatorImpl<PacketTaskType, void>
	{
		static auto createJob()
		{
			return [](std::shared_ptr<void>& data, std::any& returnedValue)
			{
				auto* work = static_cast<PacketTaskType*>(data.get());
				std::apply(work->callable, work->arguments);
			};
		}
	};


	template<typename PacketTaskType>
	struct JobCreator : public JobCreatorImpl<PacketTaskType, typename PacketTaskType::ResultType>
	{};


	template<typename Callable, typename ... Args>
	struct PacketTask
	{
		using CallableType = Callable;
		using ResultType = std::invoke_result_t<Callable, Args...>;

		template<typename Callable, typename ... Args>
		PacketTask(Callable&& f, Args&&... args) :
			callable(std::forward<Callable>(f)),
			arguments(std::forward<Args>(args)...)
		{}

		CallableType callable;
		std::tuple<Args...> arguments;
	};

}