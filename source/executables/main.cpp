#include <cstdint>
#include <array>
#include <iostream>
#include <cstdlib>

#include "../task.hpp"

#include "../task_factory.hpp"
#include "../task_executor.hpp"

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
	std::cout << "Task factory: " << std::endl;
	std::unique_ptr<TaskFactory> tf = std::make_unique<TaskFactory>();
	TaskExecuter te(2);
	{
		auto task = tf->createTask([]() -> void { std::cout << "Hello from t1!!!" << std::endl; });
		task.then([](Task<void>&) -> void { std::cout << "Hello from t2!!!" << std::endl; });
		task.then([](Task<void>&) -> int { std::cout << "Hello from t3!!!" << std::endl; return 5; });

		std::cout << "Create task executer: " << std::endl;

		te.push(task);
	}
	te.wait();
	
	std::cout << "Scope has ended" << std::endl;

	return 0;
}
