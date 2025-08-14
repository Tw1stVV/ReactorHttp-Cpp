#include "Channel.h"

Channel::Channel(int fd, FDevent events, void* arg, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc)
    :m_socket(fd), m_events((int)events), m_arg(arg),
    readCallback(readFunc), writeCallback(writeFunc), destroyCallback(destroyFunc)
{
}

void Channel::setWriteEventEnable(bool flag)
{
    if (flag)
    {
        this->m_events |= static_cast<int>(FDevent::WriteEvent);
    }
    else
    {
        this->m_events &= static_cast<int>(FDevent::WriteEvent);
    }
}

bool Channel::isWriteEventEnable()
{
    return this->m_events & static_cast<int>(FDevent::WriteEvent);
}
