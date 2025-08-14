#include "TcpServer.h"
#include "EventLoop.h"
#include "ThreadPool.h"
#include "Channel.h"
#include "TcpConnection.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>

TcpServer::TcpServer(unsigned short port, int threadNum)
	:m_port(port), m_threadNum(threadNum)
{
	m_mainLoop = new EventLoop();
	m_threadPool = new ThreadPool(m_mainLoop, m_threadNum);
	setListen();
}

TcpServer::~TcpServer()
{
    if (m_mainLoop != nullptr)
    {
        delete m_mainLoop;
    }
    if (m_threadPool != nullptr)
    {
        delete m_threadPool;
    }
}

void TcpServer::setListen()
{
    // 1. 创建监听的fd
    this->m_lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_lfd == -1)
    {
        perror("socket");
        exit(0);
    }
    // 2. 设置端口复用
    int opt = 1;
    int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    if (ret == -1)
    {
        perror("setsockopt");
        exit(0);
    }
    // 3. 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(m_lfd, (struct sockaddr*)&addr, sizeof addr);
    if (ret == -1)
    {
        perror("bind");
        exit(0);
    }
    // 4. 设置监听
    ret = listen(m_lfd, 128);
    if (ret == -1)
    {
        perror("listen");
        exit(0);
    }
}

void TcpServer::run()
{
    //启动线程池
    m_threadPool->run();
    auto obj = std::bind(&TcpServer::acceptConnection, this, std::placeholders::_1);
    Channel* channel = new Channel(m_lfd, FDevent::ReadEvent, this, 
        obj, nullptr, nullptr);
    //添加检测任务
    m_mainLoop->addTask(channel, ElemType::ADD);
    m_mainLoop->run();
}

void TcpServer::acceptConnection(void* arg)
{
    TcpServer* server = static_cast<TcpServer*>(arg);
    int cfd = accept(server->m_lfd, NULL, NULL);
    EventLoop* evLoop = server->m_threadPool->takeWorkerThreadEventLoop();
    //创建tcp连接
    TcpConnection* conn = new TcpConnection(cfd, evLoop);
}
