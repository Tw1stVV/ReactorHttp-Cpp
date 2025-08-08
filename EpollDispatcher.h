#pragma once
#include "Dispatcher.h"

struct epoll_event;


class EpollDispatcher : public Dispatcher
{
public:
	EpollDispatcher(EventLoop* evLoop);
	~EpollDispatcher();
	int add() override;
	int remove() override;
	int modify() override;
	int dispatch(int timeout) override;

private:
	int epollCtl(int op);

private:
	int m_epfd;
	epoll_event* m_events;
	const int m_maxNode = 550;
};

