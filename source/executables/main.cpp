#include <cstdint>
#include <array>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include "../task.hpp"

#include "../task_factory.hpp"
#include "../task_executor.hpp"

#include "../utils/time_utils.hpp"

class A
{
public:
	A(std::uint8_t n) :
		m_n(n)
	{
		std::cout << "A(" << static_cast<uint32_t>(m_n) << ")" << std::endl;
	}

	~A()
	{
		std::cout << "~A()" << std::endl;
	}

	void pr() const
	{
		std::cout << "pr(): " << static_cast<uint32_t>(m_n) << std::endl;
	}

private:
	std::uint8_t m_n;
};

int main(int argc, const char* argv[])
{
	using namespace std::chrono_literals;

	std::uint64_t dTime = 0;
	{
		std::unique_ptr<TaskFactory> tf = std::make_unique<TaskFactory>();
		TaskExecuter te(3);
		Task<void> task;
		Task<void> task2;
		{
			ScopedTimer t(dTime);
			task = tf->createTask([]() -> void { std::cout << "Hello from t1!!!" << std::endl; });
			task2 = task.then([](Task<void>&) -> void { std::cout << "Hello from t2!!!" << std::endl; });
			task2.then([](Task<void>&) { std::this_thread::sleep_for(1s); std::cout << "Hello from t3!!!" << std::endl; });
			task.then([](Task<void>&) -> int { std::this_thread::sleep_for(4s); std::cout << "Hello from t4!!!" << std::endl; return 5; });
			task.then([](Task<void>&) -> int { std::cout << "Hello from t5!!!" << std::endl; return 5; });
		}

		task2.wait(te);
		std::cout << "Scope has ended" << std::endl;

		te.wait();
	}

	std::cout << "Ellapsed time (n sec) : " << dTime << std::endl;

	return 0;
}
