#pragma once

class EventLoop;
class ThreadPool;

class TcpServer
{
public:
	TcpServer(unsigned short port, int threadNum);
	~TcpServer();
	void setListen();
	void run();
private:
	void acceptConnection(void* arg);

private:
	EventLoop* m_mainLoop;
	ThreadPool* m_threadPool;
	int m_threadNum;
	int m_lfd;
	unsigned short m_port;
};

