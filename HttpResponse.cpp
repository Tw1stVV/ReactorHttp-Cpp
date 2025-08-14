#include "HttpResponse.h"
#include "TcpConnection.h"
HttpResponse::HttpResponse()
	:sendDataFunc(nullptr)
{
	this->m_statusCode = HttpStatusCode::Unknown;
	m_responseHeaders.clear();
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::addHeader(std::string key, std::string value)
{
	if (key.empty() || value.empty())
	{
		return;
	}
	m_responseHeaders.insert(std::make_pair(key, value));
}

void HttpResponse::prepareMsg(Buffer* sendBuf, int cfd)
{
    // 状态行
    char tmp[1024] = { 0 };
    sprintf(tmp, "HTTP/1.1 %d %s\r\n", this->m_statusCode, this->m_info[(int)m_statusCode].data());
    sendBuf->appendString(tmp);
    // 响应头
    for (auto iter = m_responseHeaders.begin(); iter != m_responseHeaders.end(); ++iter)
    {
        sprintf(tmp, "%s: %s\r\n", iter->first.data(), iter->second.data());
        sendBuf->appendString(tmp);
    }
    // 空行
    sendBuf->appendString("\r\n");
#ifndef MSG_SEND_AUTO
    sendBuf->sendData(cfd);
#endif

    // 回复的数据
    this->sendDataFunc(this->m_fileName.data(), sendBuf, cfd);
}

void HttpResponse::setStatus(HttpStatusCode newCode)
{
    m_statusCode = newCode;
}

void HttpResponse::setFileName(std::string newFileName)
{
    m_fileName = newFileName;
}
