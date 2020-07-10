#include "semaphore.hpp"

Semaphore::Semaphore(std::uint32_t count) :
	MAX_COUNT(count),
	m_count(0)
{
};

Semaphore::Semaphore(Semaphore&& other) noexcept :
	m_count(other.m_count),
	MAX_COUNT(other.MAX_COUNT)
{

}

void Semaphore::wait() noexcept
{
	std::unique_lock lck(m_mutex);
	m_cv.wait(lck, [this]() { return this->m_count > 0; });
	--m_count;
}

void Semaphore::notify() noexcept
{
	std::unique_lock lck(m_mutex);

	if (m_count < MAX_COUNT)
	{
		++m_count;
	}

	m_cv.notify_one();
}

void Semaphore::notifyAll() noexcept
{
	{
		std::unique_lock lck(m_mutex);
		m_count = MAX_COUNT;
	}

	m_cv.notify_all();
}
