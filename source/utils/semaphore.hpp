#pragma once
#include <cstdint>
#include <mutex>
#include <condition_variable>

class Semaphore
{
public:
	Semaphore(std::uint32_t count = 1);

	Semaphore(const Semaphore&) = delete;

	Semaphore(Semaphore&& other) noexcept;

	void wait() noexcept;
	void notify() noexcept;
	void notifyAll() noexcept;

private:
	std::mutex m_mutex;
	std::condition_variable m_cv;
	const std::uint32_t MAX_COUNT;
	volatile std::uint32_t m_count;
};
