#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Buffer.h"
#include "TcpConnection.h"
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
HttpRequest::HttpRequest()
{
	this->reset();
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::reset()
{
	this->m_curState = HttpRequestState::ParseReqLine;
	this->m_method.clear();
	this->m_url.clear();
	this->m_version.clear();
	this->m_reqHeaders.clear();
}

HttpRequestState HttpRequest::getState()
{
	return m_curState;
}

void HttpRequest::addHeader(std::pair<std::string, std::string> header)
{
	if (header.first.empty() || header.second.empty())
	{
		return;
	}
	m_reqHeaders.insert(header);
}

std::string HttpRequest::getHeader(const std::string key)
{
	auto iter = m_reqHeaders.find(key);
	if (iter == m_reqHeaders.end())
	{
		return std::string();
	}
	return iter->second;
}

bool HttpRequest::parseRequestLine(Buffer* readBuf)
{
	// 读出请求行, 保存字符串结束地址
	char* end = readBuf->findCRLF();
	// 保存字符串起始地址
	char* start = readBuf->getData() + readBuf->getReadPos();
	// 请求行总长度
	int lineSize = end - start;

	if (lineSize)
	{
		auto methodFunc = std::bind(&HttpRequest::setMethod, this, std::placeholders::_1);
		start = splitRequestLine(start, end, " ", methodFunc);
		auto urlFunc = std::bind(&HttpRequest::setUrl, this, std::placeholders::_1);
		start = splitRequestLine(start, end, " ", urlFunc);
		auto versionFunc = std::bind(&HttpRequest::setVersion, this, std::placeholders::_1);
		splitRequestLine(start, end, NULL, versionFunc);

		// 解析请求头准备
		readBuf->readPosIncre(lineSize + 2);
		// 修改状态
		this->m_curState = HttpRequestState::ParseReqHeaders;
		return true;
	}
	return false;
}

bool HttpRequest::parseRequestHeader(Buffer* readBuf)
{
	char* end = readBuf->findCRLF();
	if (end != NULL)
	{
		char* start = readBuf->getData() + readBuf->getReadPos();
		int lineSize = end - start;
		// 基于: 搜索字符串
		char* middle = static_cast<char*>(memmem(start, lineSize, ": ", 2));
		if (middle != NULL)
		{
			std::string key(start, middle - start);

			std::string value(middle + 1, end - middle + 2);

			addHeader(std::make_pair(key, value));
			// 移动读数据的位置
			readBuf->readPosIncre(lineSize + 2);

		}
		else
		{
			// 请求头被解析完了, 跳过空行
			readBuf->readPosIncre(2);
			// 修改解析状态
			// 忽略 post 请求, 按照 get 请求处理
			this->m_curState = HttpRequestState::ParseReqDone;
		}
		return true;
	}
	return false;
}

bool HttpRequest::parseHttpRequest(Buffer* readBuf, HttpResponse* response, Buffer* sendBuf, int socket)
{
	bool flag = true;
	while (this->m_curState != HttpRequestState::ParseReqDone)
	{
		switch (this->m_curState)
		{
		case HttpRequestState::ParseReqLine:
			flag = parseRequestLine(readBuf);
			break;
		case HttpRequestState::ParseReqHeaders:
			flag = parseRequestHeader(readBuf);
			break;
		case HttpRequestState::ParseReqBody:
			break;
		default:
			break;
		}
		if (!flag)
		{
			return flag;
		}
		// 判断是否解析完毕了, 如果完毕了, 需要准备回复的数据
		if (this->m_curState == HttpRequestState::ParseReqDone)
		{
			// 1. 根据解析出的原始数据, 对客户端的请求做出处理
			processHttpRequest(response);
			// 2. 组织响应数据并发送给客户端
			response->prepareMsg(sendBuf, socket);
		}
	}
	this->m_curState = HttpRequestState::ParseReqLine;   // 状态还原, 保证还能继续处理第二条及以后的请求
	return flag;
}

char* HttpRequest::splitRequestLine(const char* start, const char* end, const char* sub, std::function<void(std::string)> callBack)
{
	char* space = const_cast<char*>(end);
	if (sub != NULL)
	{
		space = static_cast<char*>(memmem(start, end - start, sub, strlen(sub)));
		assert(space != NULL);
	}
	int length = space - start;
	callBack(std::string(start, length));
	return space + 1;
}

std::string HttpRequest::decodeMsg(std::string msg)
{
	std::string str = std::string();
	const char* from = msg.data();
	for (; *from != '\0'; ++from)
	{
		// isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
			// B2 == 178
			// 将3个字符, 变成了一个字符, 这个字符就是原始数据
			str.append(1, hexToDec(from[1]) * 16 + hexToDec(from[2]));

			// 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
			from += 2;
		}
		else
		{
			// 字符拷贝, 赋值
			str.append(1, *from);
		}
	}
	str.append(1, '\0');
	return str;
}

const char* HttpRequest::getFileType(const std::string name)
{
	// a.jpg a.mp4 a.html
	// 自右向左查找‘.’字符, 如不存在返回NULL
	const char* dot = strrchr(name.data(), '.');
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// 纯文本
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset=utf-8";
}

void HttpRequest::sendDir(std::string dirName, Buffer* sendBuf, int cfd)
{
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName.data());
	struct dirent** namelist;
	int num = scandir(dirName.data(), &namelist, NULL, alphasort);
	for (int i = 0; i < num; ++i)
	{
		// 取出文件名 namelist 指向的是一个指针数组 struct dirent* tmp[]
		char* name = namelist[i]->d_name;
		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName.data(), name);
		stat(subPath, &st);
		if (S_ISDIR(st.st_mode))
		{
			// a标签 <a href="">name</a>
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else
		{
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		// send(cfd, buf, strlen(buf), 0);
		sendBuf->appendString(buf);
#ifndef MSG_SEND_AUTO
		sendBuf->sendData(cfd);
#endif
		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	sprintf(buf, "</table></body></html>");
	// send(cfd, buf, strlen(buf), 0);
	sendBuf->appendString(buf);
#ifndef MSG_SEND_AUTO
	sendBuf->sendData(cfd);
#endif
	free(namelist);
}

void HttpRequest::sendFile(std::string fileName, Buffer* sendBuf, int cfd)
{
	// 1. 打开文件
	int fd = open(fileName.data(), O_RDONLY);
	assert(fd > 0);
#if 1
	while (1)
	{
		char buf[1024];
		int len = read(fd, buf, sizeof buf);
		if (len > 0)
		{
			// send(cfd, buf, len, 0);
			sendBuf->appendString(buf, len);
#ifndef MSG_SEND_AUTO
			sendBuf->sendData(cfd);
#endif
		}
		else if (len == 0)
		{
			break;
		}
		else
		{
			close(fd);
			perror("read");
		}
	}
#else
	off_t offset = 0;
	int size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	while (offset < size)
	{
		int ret = sendfile(cfd, fd, &offset, size - offset);
		printf("ret value: %d\n", ret);
		if (ret == -1 && errno == EAGAIN)
		{
			printf("没数据...\n");
		}
	}
#endif
	close(fd);
}

int HttpRequest::hexToDec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
}

bool HttpRequest::processHttpRequest(HttpResponse* response)
{
	if (this->m_method != "GET")
	{
		return false;
	}
	this->m_url = decodeMsg(this->m_url);
	// 处理客户端请求的静态资源(目录或者文件)
	const char* file = NULL;
	if (strcmp(this->m_url.data(), "/") == 0)
	{
		file = "./";
	}
	else
	{
		file = this->m_url.data() + 1;
	}
	// 获取文件属性
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1)
	{
		// 文件不存在 -- 回复404
		//sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		//sendFile("404.html", cfd);
		response->setFileName(std::string("404.html"));
		response->setStatus(HttpStatusCode::NotFound);
		// 响应头
		response->addHeader("Content-type", getFileType(".html"));
		//auto func = std::bind(&HttpRequest::sendFile, this, _1, _2, _3);
		response->sendDataFunc = sendFile;

		return false;
	}

	response->setFileName(file);
	response->setStatus(HttpStatusCode::OK);
	// 判断文件类型
	if (S_ISDIR(st.st_mode))
	{
		// 把这个目录中的内容发送给客户端
		//sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		//sendDir(file, cfd);
		// 响应头
		response->addHeader("Content-type", getFileType(".html"));
		//auto func = std::bind(&HttpRequest::sendDir, this, _1, _2, _3);
		response->sendDataFunc = sendDir;
		return true;
	}
	else
	{
		// 把文件的内容发送给客户端
		//sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		//sendFile(file, cfd);
		// 响应头
		char tmp[12] = { 0 };
		sprintf(tmp, "%ld", st.st_size);
		response->addHeader("Content-type", getFileType(file));
		response->addHeader("Content-length", tmp);
		//auto func = std::bind(&HttpRequest::sendFile, this, _1, _2, _3);
		response->sendDataFunc = sendFile;
		return true;
	}

	return false;
}

void HttpRequest::setMethod(std::string method)
{
	m_method = method;
}

void HttpRequest::setUrl(std::string url)
{
	m_url = url;
}

void HttpRequest::setVersion(std::string version)
{
	m_version = version;
}
