#pragma once

#include <queue>
#include <mutex>

class TaskGroup;

class TaskGroupQueue
{
public:
	void push(TaskGroup* group);
	bool execute();
	size_t getSize() const;

private:
	std::queue<TaskGroup*> m_queue;
	mutable std::mutex m_queueMutex;
};
