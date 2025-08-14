#pragma once
#include <stdbool.h>
#include <functional>

//函数指针
//using handleFunc = int(*)(void* arg);

//fd读写事件
enum class FDevent
{
	TimeOut = 0x01,
	ReadEvent = 0x02,
	WriteEvent = 0x04
};

class Channel
{
public:
	using handleFunc = std::function<void(void*)>;
	Channel(int fd, FDevent events, void* arg, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc);
	void setWriteEventEnable(bool flag);
	bool isWriteEventEnable();
	//事件回调函数
	handleFunc readCallback;
	handleFunc writeCallback;
	handleFunc destroyCallback;

	inline int getSocket()
	{
		return m_socket;
	}

	inline int getEvents()
	{
		return m_events;
	}
	inline void* getArg()
	{
		return m_arg;
	}

private:
	int m_socket;		//通信文件描述符
	int m_events;	//事件
	void* m_arg;	//回调函数参数
};