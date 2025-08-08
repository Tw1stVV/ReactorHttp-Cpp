#include "WorkerThread.h"
#include <string>

WorkerThread::WorkerThread(int index)
{
	m_thread = nullptr;
	evLoop = nullptr;
	m_threadID = std::thread::id();
	m_name = "WorkerThread-" + std::to_string(index);
}