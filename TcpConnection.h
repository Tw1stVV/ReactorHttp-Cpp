#pragma once
#include <string>
class EventLoop;
class Channel;
class Buffer;
class HttpRequest;
class HttpResponse;
#define MSG_SEND_AUTO

class TcpConnection
{
public:
    TcpConnection(int fd, EventLoop* evLoop);
    ~TcpConnection();
    std::string getName();
private:
    static void processRead(void* arg);
    static void processWrite(void* arg);
    static void destroy(void* arg);

private:
    EventLoop* m_evLoop;
    Channel* m_channel;
    Buffer* m_readBuf;
    Buffer* m_writeBuf;
    std::string m_name;
    // http 协议
    HttpRequest* m_request;
    HttpResponse* m_response;
};

