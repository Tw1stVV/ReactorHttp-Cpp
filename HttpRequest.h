#pragma once
#include <map>
#include <string>
#include <functional>

class Buffer;
class HttpResponse;

enum class HttpRequestState:char
{
    ParseReqLine,
    ParseReqHeaders,
    ParseReqBody,
    ParseReqDone
};

class HttpRequest
{
public:
    // 初始化 
    HttpRequest();

    ~HttpRequest();

    // 重置
    void reset();

    // 获取处理状态
    enum HttpRequestState getState();

    // 添加请求头
    void addHeader(std::pair<std::string, std::string> header);
    
    //获取请求头的值
    std::string getHeader(const std::string key);

    // 解析请求行
    bool parseRequestLine(Buffer* readBuf);

    // 解析请求头
    bool parseRequestHeader(Buffer* readBuf);

    // 解析http请求协议
    bool parseHttpRequest(Buffer* readBuf, HttpResponse* response, Buffer* sendBuf, int socket);

    // 处理http请求协议
    bool processHttpRequest(HttpResponse* response);

    void setMethod(std::string method);
    void setUrl(std::string url);
    void setVersion(std::string version);

private:
    char* splitRequestLine(const char* start, const char* end, const char* sub, 
        std::function<void(std::string)> callBack);
    // 解码字符串
    std::string decodeMsg(std::string from);
    const char* getFileType(const std::string name);
    static void sendDir(std::string dirName, Buffer* sendBuf, int cfd);
    static void sendFile(std::string fileName, Buffer* sendBuf, int cfd);
    int hexToDec(char c);
private:
    std::string m_method;
    std::string m_url;
    std::string m_version;
    std::map<std::string, std::string> m_reqHeaders;
    HttpRequestState m_curState;
};

