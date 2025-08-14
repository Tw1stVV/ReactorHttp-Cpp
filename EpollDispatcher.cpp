#include "EpollDispatcher.h"
#include "EventLoop.h"
#include <sys/epoll.h>
#include <stdio.h>
#include <cstdlib>
#include "Channel.h"
#include <unistd.h>

EpollDispatcher::EpollDispatcher(EventLoop* evLoop)
    :Dispatcher(evLoop)
{
    this->m_epfd = epoll_create(10);
    if (this->m_epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }
    this->m_events = new epoll_event[m_maxNode];
}

EpollDispatcher::~EpollDispatcher()
{
    close(this->m_epfd);
    delete[] this->m_events;
}

int EpollDispatcher::add()
{
    int ret = epollCtl(EPOLL_CTL_ADD);
    if (ret == -1)
    {
        perror("epoll_crl add");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::remove()
{
    int ret = epollCtl(EPOLL_CTL_DEL);
    if (ret == -1)
    {
        perror("epoll_crl delete");
        exit(0);
    }
    // 通过 channel 释放对应的 TcpConnection 资源
    m_channel->destroyCallback(m_channel->getArg());
    return ret;
}

int EpollDispatcher::modify()
{
    int ret = epollCtl(EPOLL_CTL_MOD);
    if (ret == -1)
    {
        perror("epoll_crl modify");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::dispatch(int timeout)
{
    int count = epoll_wait(m_epfd, m_events, m_maxNode, timeout * 1000);
    for (int i = 0; i < count; ++i)
    {
        int events = m_events[i].events;
        int fd = m_events[i].data.fd;
        if (events & EPOLLERR || events & EPOLLHUP)
        {
            continue;
        }
        if (events & EPOLLIN)
        {
            m_evLoop->active(fd, (int)FDevent::ReadEvent);
        }
        if (events & EPOLLOUT)
        {
            m_evLoop->active(fd, (int)FDevent::WriteEvent);
        }
    }
    return 0;
}

int EpollDispatcher::epollCtl(int op)
{
    struct epoll_event ev;
    ev.data.fd = m_channel->getSocket();
    int events = 0;
    if (m_channel->getEvents() & (int)FDevent::ReadEvent)
    {
        events |= EPOLLIN;
    }
    if (m_channel->getEvents() & (int)FDevent::WriteEvent)
    {
        events |= EPOLLOUT;
    }
    ev.events = events;
    int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
    return ret;
}
