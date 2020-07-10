#pragma once
#include <cstdint>
#include <chrono>

class ScopedTimer
{
public:
	explicit ScopedTimer(std::uint64_t& timeStore) noexcept;
	~ScopedTimer();

private:
	std::chrono::high_resolution_clock::time_point m_start;
	std::uint64_t& m_timeStorage;
};

class ManualTimer
{
public:
	void start() noexcept;
	std::uint64_t end() noexcept;
	void reset() noexcept;

private:
	std::chrono::high_resolution_clock::time_point m_start;
};
