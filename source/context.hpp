#pragma once

#include <cstdint>
#include <memory>
#include <any>

#include "private/job_creator.hpp"
#include "private/handle_array.hpp"

using ContextID = std::uint16_t;

struct Context
{
	void(*job)(std::shared_ptr<void>&, std::any&);
	std::shared_ptr<void> data;
	std::any returnedValue;
};

template<std::uint32_t PoolSize>
class ContextPool
{
public:
	template<typename Callable, typename ... Args>
	std::uint16_t createContext(Callable&& callable, Args&&... args)
	{
		using DataType = Detail::PacketTask<Callable, Args ...>;
		return m_pool.emplace(Detail::JobCreator<DataType>::createJob(),
			std::make_shared<DataType>(std::forward<Callable>(callable), std::forward<Args>(args)...),
			std::any());
	}

	Context& get(std::uint16_t handle) noexcept
	{
		return m_pool.get(handle);
	}

	const Context& get(std::uint16_t handle) const noexcept
	{
		return m_pool.get(handle);
	}

private:
	HandleArray<Context, std::uint16_t, PoolSize> m_pool;
};