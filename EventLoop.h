#pragma once
#include <thread>
#include <mutex>
#include <map>
#include <queue>

class Channel;
class Dispatcher;


//任务类型
enum class ElemType:char { ADD, DELETE, MODIFY };

// 定义任务队列的节点
struct ChannelElement
{
	ElemType type;   // 如何处理该节点中的channel
	Channel* channel;
};

class EventLoop
{
public:
	EventLoop();
	EventLoop(const std::string threadName);
	~EventLoop();

	int run();	//启动反应堆模型
	int active(int fd, int event);	//事件响应处理
	int addTask(Channel* channel, ElemType type);	//添加任务
	int processTask();	//处理任务队列任务
	int freeChannel(Channel* channel);	//释放Channel
	std::thread::id getThreadID();
	std::string getThreadName();
	std::string getThreadID_s();

private:
	void readLocalMessage();
	int add(Channel* channel);
	int remove(Channel* channel);
	int modify(Channel* channel);
	void taskWakeup();

private:
	bool m_isQuit;							//标记反应堆是否工作
	Dispatcher* m_dispatcher;			//反应堆模型
	std::queue<ChannelElement*> m_taskQueue;	// 任务队列
	std::map<int, Channel*> m_channelMap;	// 存储fd对应的channel

	// 线程id, name, mutex
	std::thread::id m_threadID;
	std::string m_threadName;
	std::mutex m_mutex;
	int m_socketPair[2];  // 存储本地通信的fd 通过socketpair 初始化
};

