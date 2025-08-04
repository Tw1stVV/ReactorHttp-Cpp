#pragma once
#include <stdbool.h>

//函数指针
using handleFunc = int(*)(void* arg);

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
	Channel(int fd, int events, handleFunc readFunc, handleFunc write, handleFunc destroy, void* arg);
	void setWriteEventEnable(bool flag);
	bool isWriteEventEnable();
	//事件回调函数
	handleFunc readCallback;
	handleFunc writeCallback;
	handleFunc destroyCallback;
private:
	int fd;		//通信文件描述符
	int events;	//事件
	void* arg;	//回调函数参数
};