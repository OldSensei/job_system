#pragma once
#include <condition_variable>
#include <mutex>

class Event
{
public:
	Event() = default;

	Event(const Event&) = delete;
	Event& operator=(const Event&) = delete;

	void wait()
	{
		std::unique_lock lock{m_mutex};
		m_cv.wait(lock, [this]()->bool { return m_signaled; });
	}

	void notify()
	{
		{
			std::unique_lock lock{ m_mutex };
			m_signaled = true;
		}

		m_cv.notify_all();
	}

private:
	std::mutex m_mutex;
	std::condition_variable m_cv;
	volatile bool m_signaled = false;
};
