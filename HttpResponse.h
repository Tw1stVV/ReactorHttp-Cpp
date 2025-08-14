#pragma once
#include <map>
#include <string>
#include <functional>
#include "Buffer.h"
// 响应状态码
enum class HttpStatusCode
{
	Unknown,
	OK = 200,
	MovedPermanently = 301,
	MovedTemporarily = 302,
	BadRequest = 400,
	NotFound = 404
};

class HttpResponse
{
public:
	std::function<void(std::string fileName, Buffer* sendBuf, int cfd)> sendDataFunc;
	HttpResponse();
	~HttpResponse();
	void addHeader(std::string key, std::string value);
	void prepareMsg(Buffer* sendBuf, int cfd);
	void setStatus(HttpStatusCode newCode);
	void setFileName(std::string newFileName);
private:
	enum HttpStatusCode m_statusCode;
	std::string m_fileName;
	std::map<std::string, std::string> m_responseHeaders;
	std::map<int, std::string>m_info =
	{
		{200, "OK"},
		{301, "MovedPermanently"},
		{302, "MovedTemporarily"},
		{400, "BadRequest"},
		{404, "NotFound"}
	};
};

