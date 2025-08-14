#include "EventLoop.h"
#include "EpollDispatcher.h"
#include "Channel.h"
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <string>
#include <sstream>

EventLoop::EventLoop()
	:EventLoop(std::string())
{
}

EventLoop::EventLoop(const std::string threadName)
{
	this->m_isQuit = true;	//默认关闭
	this->m_dispatcher = new EpollDispatcher(this);
	this->m_threadID = std::this_thread::get_id();
	this->m_threadName = threadName.empty() ? "MainThread" : threadName;
	this->m_channelMap.clear();

	int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
	if (ret == -1)
	{
		perror("socketpair errro:");
		exit(0);
	}
	auto obj = std::bind(&EventLoop::readLocalMessage, this);
	Channel* channel = new Channel(m_socketPair[1], FDevent::ReadEvent, this, 
		obj, nullptr, nullptr);
	
	addTask(channel, ElemType::ADD);
}

EventLoop::~EventLoop()
{
}

int EventLoop::run()
{
	this->m_isQuit = false;
	// 比较线程ID是否正常
	if (this->m_threadID != std::this_thread::get_id())
	{
		return -1;
	}
	// 循环进行事件处理
	while (!this->m_isQuit)
	{
		this->m_dispatcher->dispatch(2);    // 超时时长 2s
		this->processTask();
	}
}

int EventLoop::active(int fd, int event)
{
	if (fd < 0)
	{
		return -1;
	}
	// 取出channel
	struct Channel* channel = this->m_channelMap[fd];
	if (event & (int)FDevent::ReadEvent && channel->readCallback)
	{
		channel->readCallback(channel->getArg());
	}
	if (event & (int)FDevent::WriteEvent && channel->writeCallback)
	{
		channel->writeCallback(channel->getArg());
	}
	return 0;
}

int EventLoop::addTask(Channel* channel, ElemType type)
{
	// 加锁, 保护共享资源
	this->m_mutex.lock();
	struct ChannelElement* node = new ChannelElement();	// 创建新任务
	node->channel = channel;
	node->type = type;
	this->m_taskQueue.push(node);	//添加任务
	this->m_mutex.unlock();

	// 处理节点
	/*
	*   1. 对于链表节点的添加: 可能是当前线程也可能是其他线程(主线程)
	*       1). 修改fd的事件, 当前子线程发起, 当前子线程处理
	*       2). 添加新的fd, 添加任务节点的操作是由主线程发起的
	*   2. 不能让主线程处理任务队列, 需要由当前的子线程取处理
	*/
	if (this->m_threadID == std::this_thread::get_id())
	{
		// 当前子线程(基于子线程的角度分析)
		processTask();
	}
	else
	{
		// 主线程 -- 告诉子线程处理任务队列中的任务
		// 1. 子线程在工作 2. 子线程被阻塞了:select, poll, epoll
		taskWakeup();
	}
	return 0;
}

int EventLoop::processTask()
{
	
	while (!this->m_taskQueue.empty())
	{
		this->m_mutex.lock();
		ChannelElement* head = this->m_taskQueue.front();
		this->m_taskQueue.pop();
		this->m_mutex.unlock();

		Channel* channel = head->channel;
		if (head->type == ElemType::ADD)
		{
			// 添加
			add(channel);
		}
		else if (head->type == ElemType::DELETE)
		{
			// 删除
			remove(channel);
		}
		else if (head->type == ElemType::MODIFY)
		{
			// 修改
			modify(channel);
		}
		delete head;
	}
	return 0;
}

int EventLoop::freeChannel(Channel* channel)
{
	// 删除 channel 和 fd 的对应关系
	std::map<int, Channel*>::iterator iter = this->m_channelMap.find(channel->getSocket());
	this->m_channelMap.erase(iter);
	// 关闭 fd
	close(channel->getSocket());
	// 释放 channel
	delete channel;
	return 0;
}

std::thread::id EventLoop::getThreadID()
{
	return m_threadID;
}

std::string EventLoop::getThreadName()
{
	return m_threadName;
}

std::string EventLoop::getThreadID_s()
{
	std::stringstream ss;
	ss << m_threadID;
	return ss.str();
}

void EventLoop::readLocalMessage()
{
	char buf[256];
	read(this->m_socketPair[1], buf, sizeof(buf));
}

int EventLoop::add(Channel* channel)
{
	if(this->m_channelMap.find(channel->getSocket()) == this->m_channelMap.end())
	{
		this->m_channelMap.insert(std::make_pair(channel->getSocket(), channel));
		this->m_dispatcher->setChannel(channel);
		this->m_dispatcher->add();
		return 0;
	}
	return -1;
}

int EventLoop::remove(Channel* channel)
{
	auto iter = this->m_channelMap.find(channel->getSocket());
	if (iter != this->m_channelMap.end())
	{
		this->m_channelMap.erase(iter);
		this->m_dispatcher->setChannel(channel);
		this->m_dispatcher->remove();
		return 0;
	}
	return -1;
}

int EventLoop::modify(Channel* channel)
{
	auto iter = this->m_channelMap.find(channel->getSocket());
	if (iter != this->m_channelMap.end())
	{
		this->m_dispatcher->setChannel(channel);
		this->m_dispatcher->modify();
	}
	return -1;
}

void EventLoop::taskWakeup()
{
	const char* msg = "从水下第一个生命的萌芽开始，到石器时代的巨型野兽，再到人类第一次直立行走，你已经历许多。现在，开启你伟大的探索吧：从早期文明的摇篮，到浩瀚星宇。!!!";
	write(this->m_socketPair[0], msg, strlen(msg));
}
