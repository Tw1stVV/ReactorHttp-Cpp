#pragma once
class EventLoop;
class Channel;

class Dispatcher
{
public:
	Dispatcher(EventLoop* evLoop);
	~Dispatcher();

	virtual int add() = 0;

	virtual int remove() = 0;

	virtual int modify() = 0;

	virtual int dispatch(int timeout = 2) = 0;//timeout单位毫秒

	virtual void setChannel(Channel* channel);
protected:
	EventLoop* m_evLoop;
	Channel* m_channel;
};