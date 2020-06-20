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

int main(int argc, char* argv[])
{
	std::cout << "Task factory: " << std::endl;
	TaskFactory tf;
	auto task = tf.createTask([]() -> void { std::cout << "Hello from t1!!!" << std::endl; });
	task.then([](Task<void>&) -> void { std::cout << "Hello from t2!!!" << std::endl; });

	std::cout << "Create task executer: " << std::endl;
	TaskExecuter te(2);
	te.push(task);
	te.wait();
	//auto t = createTask([]() -> void { std::cout << "Hello from t1!!!" << std::endl; });
	//t.then([](Task<void>&)->void { std::cout << "Hello from t2!!!" << std::endl; });
	//auto t2 = t.then([](Task<void>&)->int { std::cout << "Hello from t3!!!" << std::endl; return 2; });
	//t2.then([](Task<int>& task)->void { std::cout << "Hello from t4: " << task.get() << std::endl; });
	//t2.then([](Task<int>& task)->void { std::cout << "Hello from t4: " << task.get() << std::endl; });
	
	//std::cout << "========create executer============" << std::endl;
	//TaskExecuter te(2);
	//te.push(t);
	//te.wait();

	return 0;
}
