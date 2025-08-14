#include "TcpConnection.h"
#include "Buffer.h"
#include "EventLoop.h"
#include "Channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Log.h"

TcpConnection::TcpConnection(int fd, EventLoop* evLoop)
{
	this->m_evLoop = evLoop;
	this->m_readBuf = new Buffer(10240);
	this->m_writeBuf = new Buffer(10240);

	this->m_request = new HttpRequest();
	this->m_response = new HttpResponse();
	this->m_channel = new Channel(fd, FDevent::ReadEvent, this, 
        &TcpConnection::processRead, &TcpConnection::processWrite, &TcpConnection::destroy);
	evLoop->addTask(m_channel, ElemType::ADD);
	m_name = std::string("connection-" + fd);
	Debug("和客户端建立连接, threadName: %s, threadID:%s, connName: %s",
		evLoop->getThreadName().data(), evLoop->getThreadID_s(), this->m_name.data());
}

TcpConnection::~TcpConnection()
{
    if (m_readBuf && m_readBuf->readableSize() == 0 &&
        m_writeBuf && m_writeBuf->readableSize() == 0)
    {
        m_evLoop->freeChannel(m_channel);
        delete m_readBuf;
        delete m_writeBuf;
        delete m_request;
        delete m_response;
    }
    Debug("连接断开, 释放资源, ConnetionName: %s", m_name.data());
}

std::string TcpConnection::getName()
{
    return m_name;
}

void TcpConnection::processRead(void* arg)
{
    TcpConnection* conn = static_cast<TcpConnection*>(arg);
    // 接收数据
    int count = conn->m_readBuf->socketRead(conn->m_channel->getSocket());

    Debug("接收到的http请求数据: %s", conn->m_readBuf->getData() + conn->m_readBuf->getReadPos());
    if (count > 0)
    {
        // 接收到了 http 请求, 解析http请求
        int socket = conn->m_channel->getSocket();
#ifdef MSG_SEND_AUTO
        conn->m_channel->setWriteEventEnable(true);
        conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
#endif
        bool flag = conn->m_request->parseHttpRequest(conn->m_readBuf, conn->m_response, conn->m_writeBuf, socket);
        if (!flag)
        {
            // 解析失败, 回复一个简单的html
            char* errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
            conn->m_writeBuf->appendString(errMsg);
        }
    }
    else
    {
#ifdef MSG_SEND_AUTO
        // 断开连接
        conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif

    }
#ifndef MSG_SEND_AUTO
    // 断开连接
    conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif
}

void TcpConnection::processWrite(void* arg)
{
    TcpConnection* conn = static_cast<TcpConnection*>(arg);

    Debug("开始发送数据(基于写事件发送)....");
    // 发送数据
    int count = conn->m_writeBuf->sendData(conn->m_channel->getSocket());
    if (count > 0)
    {
        // 判断数据是否被全部发送出去了
        if (conn->m_writeBuf->readableSize() == 0)
        {
            // 1. 不再检测写事件 -- 修改channel中保存的事件
            conn->m_channel->setWriteEventEnable(false);
            // 2. 修改dispatcher检测的集合 -- 添加任务节点
            conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
            // 3. 删除这个节点
            conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
        }
    }
    return;
}

void TcpConnection::destroy(void* arg)
{
    TcpConnection* conn = static_cast<TcpConnection*>(arg);
    if (conn != nullptr)
    {
        delete conn;
    }
}
