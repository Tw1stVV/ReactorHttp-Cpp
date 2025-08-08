#include "Dispatcher.h"

Dispatcher::Dispatcher(EventLoop* evLoop)
	:m_evLoop(evLoop), m_channel(nullptr)
{
}

Dispatcher::~Dispatcher()
{
}

void Dispatcher::setChannel(Channel* channel)
{
	m_channel = channel;
}



