#include "Channel.h"

Channel::Channel(int fd, int events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg)
	:fd(fd), events(events), 
	readCallback(readFunc), writeCallback(writeFunc), destroyCallback(destroyFunc), arg(arg)
{

}

void Channel::setWriteEventEnable(bool flag)
{
    if (flag)
    {
        this->events |= static_cast<int>(FDevent::WriteEvent);
    }
    else
    {
        this->events &= static_cast<int>(FDevent::WriteEvent);
    }
}

bool Channel::isWriteEventEnable()
{
    return this->events & FDevent::WriteEvent;
}
