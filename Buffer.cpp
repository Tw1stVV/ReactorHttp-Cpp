#include "Buffer.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <string.h>
Buffer::Buffer(int size)
	:m_capacity(size)
{
	m_data = (char*)malloc(size);
	bzero(m_data, size);
}

Buffer::~Buffer()
{
	if (m_data != nullptr)
	{
		free(m_data);
	}
}

void Buffer::extendRoom(int size)
{
    // 1. 内存够用 - 不需要扩容
    if (writeableSize() >= size)
    {
        return;
    }
    // 2. 内存需要合并才够用 - 不需要扩容
    // 剩余的可写的内存 + 已读的内存 > size
    else if (this->m_readPos + writeableSize() >= size)
    {
        // 得到未读的内存大小
        int readable = readableSize();
        // 移动内存
        memcpy(this->m_data, this->m_data + this->m_readPos, readable);
        // 更新位置
        this->m_readPos = 0;
        this->m_writePos = readable;
    }
    // 3. 内存不够用 - 扩容
    else
    {
        void* temp = realloc(this->m_data, this->m_capacity + size);
        if (temp == NULL)
        {
            return; // 失败了
        }
        memset(temp + this->m_capacity, 0, size);
        // 更新数据
        this->m_data = static_cast<char*>(temp);
        this->m_capacity += size;
    }
}

int Buffer::writeableSize()
{
    return m_capacity - m_writePos;
}

int Buffer::readableSize()
{
    return m_capacity - m_readPos;
}

int Buffer::appendString(const char* data)
{
    int size = strlen(data);
    int ret = appendString(data, size);
    return ret;
}

int Buffer::appendString(const char* data, int size)
{
    if (m_data == nullptr || size <= 0)
    {
        return -1;
    }
    // 扩容
    extendRoom(size);
    // 数据拷贝
    memcpy(this->m_data + this->m_writePos, data, size);
    this->m_writePos += size;
    return 0;
}

int Buffer::socketRead(int fd)
{
    // read/recv/readv
    struct iovec vec[2];
    // 初始化数组元素
    int writeable = writeableSize();
    vec[0].iov_base = this->m_data + this->m_writePos;
    vec[0].iov_len = writeable;
    char* tmpbuf = (char*)malloc(40960);
    vec[1].iov_base = tmpbuf;
    vec[1].iov_len = 40960;
    int result = readv(fd, vec, 2);
    if (result == -1)
    {
        return -1;
    }
    else if (result <= writeable)
    {
        this->m_writePos += result;
    }
    else
    {
        this->m_writePos = this->m_capacity;
        appendString(tmpbuf, result - writeable);
    }
    free(tmpbuf);
    return result;
}

char* Buffer::findCRLF()
{
    void* ptr = memmem(this->m_data + this->m_readPos, readableSize(), "\r\n", 2);
    return static_cast<char*>(ptr);
}

int Buffer::sendData(int socket)
{
    int readable = readableSize();
    if (readable > 0)
    {
        int count = send(socket, this->m_data + this->m_readPos, readable, MSG_NOSIGNAL);
        if (count > 0)
        {
            this->m_readPos += count;
            usleep(1);
        }
        return count;
    }
    return 0;
}

int Buffer::readPosIncre(int count)
{
    m_readPos += count;
    return m_readPos;
}

char* Buffer::getData()
{
    return m_data;
}

void Buffer::setData(char* newData)
{
    m_data = newData;
}

int Buffer::getReadPos()
{
    return m_readPos;
}

int Buffer::getWritePos()
{
    return m_writePos;
}

void Buffer::setReadPos(int newPos)
{
    m_readPos = newPos;
}

void Buffer::setWritePos(int newPos)
{
    m_writePos = newPos;
}
