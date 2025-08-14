#pragma once
class Buffer
{
public:
    Buffer(int size);
    ~Buffer();
    // 扩容
    void extendRoom(int size);
    // 得到剩余的可写的内存容量
    int writeableSize();
    // 得到剩余的可读的内存容量
    int readableSize();
    // 写内存 1. 直接写 2. 接收套接字数据
    int appendString(const char* data);
    int appendString(const char* data, int size);
    int socketRead(int fd);
    // 根据\r\n取出一行, 找到其在数据块中的位置, 返回该位置
    char* findCRLF();
    // 发送数据
    int sendData(int socket);

    int readPosIncre(int count);
    char* getData();
    void setData(char* newData);
    int getReadPos();
    int getWritePos();
    void setReadPos(int newPos);
    void setWritePos(int newPos);
private:
    char* m_data;
    int m_capacity;
    int m_readPos = 0;
    int m_writePos = 0;
};

