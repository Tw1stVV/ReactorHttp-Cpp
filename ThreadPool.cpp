#include "ThreadPool.h"
#include "WorkerThread.h"
#include "EventLoop.h"

ThreadPool::ThreadPool(EventLoop* mainLoop, int count)
{
	m_threadNum = count;
	index = 0;
	isStart = false;
	m_mainLoop = mainLoop;
	m_workerThreads.clear();
}

void ThreadPool::run()
{
	if (std::this_thread::get_id() != m_mainLoop->getThreadID())
	{
		exit(0);
	}
	isStart = true;
	for (int i = 0; i < m_threadNum; ++i)
	{
		WorkerThread* p = new WorkerThread(i);
		m_workerThreads.push_back(p);
		p->run();
	}
}

EventLoop* ThreadPool::takeWorkerThreadEventLoop()
{
	if (std::this_thread::get_id() != m_mainLoop->getThreadID() ||
		!isStart)
	{
		exit(0);
	}

	if (m_threadNum == 0)
	{
		return m_mainLoop;
	}

	auto ptr = m_workerThreads[index]->getEventLoop();
	index = ++index % m_threadNum;
	return ptr;
}