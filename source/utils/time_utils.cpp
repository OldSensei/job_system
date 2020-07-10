#include "time_utils.hpp"

ScopedTimer::ScopedTimer(std::uint64_t& timeStore) noexcept : 
	m_timeStorage{ timeStore },
	m_start{ std::chrono::high_resolution_clock::now() }
{
	
}

ScopedTimer::~ScopedTimer()
{
	auto end = std::chrono::high_resolution_clock::now();
	auto result = std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start);
	m_timeStorage = result.count();
}

void ManualTimer::start() noexcept
{
	m_start = std::chrono::high_resolution_clock::now();
}

std::uint64_t ManualTimer::end() noexcept
{
	auto end = std::chrono::high_resolution_clock::now();
	auto result = std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start);
	return result.count();
}

void ManualTimer::reset() noexcept
{
	m_start = std::chrono::high_resolution_clock::now();
}
