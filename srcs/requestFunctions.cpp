#include "WebServer.hpp"

#define TIMEOUT_MS 5000

static void	epollMood(int fd, uint32_t mood)
{
	struct epoll_event ev;
	ev.events = mood;
	ev.data.fd = fd;
	epoll_ctl(conf::epfd(), EPOLL_CTL_MOD, fd, &ev);
}

static void trim(std::string& s)
{
	size_t start = 0;
	while (start < s.size() && std::isspace(s[start])) start++;
	size_t end = s.size();
	while (end > start && std::isspace(s[end - 1])) end--;
	s = s.substr(start, end - start);
}

StatusCode	request::myRead(int fd, std::string& _buffer)
{
	char buffer[1024];

	int len = read(fd, buffer, sizeof(buffer));

	if (len < 0)
		return READ_ERROR;

	if (len == 0)
		return CLIENT_DISCONECTED;

	_buffer.append(buffer, len);
 
	return REPEAT;
}

StatusCode	request::endNode()	{ return END; }

static bool timeoutCheck(struct timeval *last_activity, long timeout_ms)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    long seconds  = now.tv_sec  - last_activity->tv_sec;
    long usec     = now.tv_usec - last_activity->tv_usec;

    long elapsed_ms = seconds * 1000 + usec / 1000;

    return (elapsed_ms > timeout_ms);
}

StatusCode request::execNode(nodeData& data, const nodeHandler nodes[])
{
	StatusCode code;

	while (1)
	{
		if (data.timeout == ON && timeoutCheck(&_last_activity, TIMEOUT_MS))
			return REQUEST_TIMEOUT;

		code = (this->*(nodes[data.currentNode].handler))();

		if (code == END)
			return FINISHED;

		if (code != FINISHED || data.cascade == OFF)
			break;
		
		if (data.timeout == ON)
			gettimeofday(&_last_activity, NULL);	// Reset the timeout per action finished
	
		data.currentNode = static_cast<Nodes>(static_cast<int>(data.currentNode) + 1);
	}
	return code;
}

StatusCode	request::setUpRequestLine()
{
	size_t header_end = _buffer.find("\n");

	if (header_end != std::string::npos)
	{
		std::istringstream iss(_buffer);

		if (!(iss >> _method >> _path >> _protocol))
			return BAD_REQUEST;

		_buffer.erase(0, header_end + 1);

		_location = _server->getLocation(_path);

		if (!_location.methodAllowed(_method))
			return METHOD_NOT_ALLOWED;

		if (_path.find(_location.getPath()) == 0)	// In case location path is prefix of target
			_path = _location.getRoot() + _path.substr(_location.getPath().size());
		else
			_path = _location.getRoot() + _path;
		
		return FINISHED;
	}
	return REPEAT;
}

StatusCode	request::setUpHeader()
{
	while (true)
	{
		size_t end = _buffer.find("\r\n");
		if (end == std::string::npos)
			return REPEAT;

		std::string line = _buffer.substr(0, end);
		_buffer.erase(0, end + 2);

		if (line.empty())
			return FINISHED;

		size_t colon = line.find(':');
		if (colon == std::string::npos)
			return BAD_REQUEST;

		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);
		trim(key);
		trim(value);

		_headers[key] = value;
	}
}

StatusCode	request::setUpBody()
{
	if (_buffer.length() >= _contentLength)
	{
		_body = _buffer;
		return FINISHED;
	}
	if (_buffer.length() >= _location.getBodySize())
		return BAD_REQUEST;
	return REPEAT;
}

StatusCode	request::readResponse()
{
	return myRead(_infile, _buffer);
}

StatusCode	request::autoindex()
{
	std::cout << "AUTOINDEX" << std::endl;
	return FINISHED;
}

//std::string		sendChunkedHeader(int _fd, StatusCode code, std::string fileType)
//{
//	std::ostringstream response;
//	response << "HTTP/1.1 200 OK\r\n";
//	response << "Content-Type: " << fileType << "\r\n";
//	response << "Transfer-Encoding: chunked\r\n" << "\r\n";
//	response << "Connection: close\r\n";
//	response << "\r\n";
//
//	send(_fd, response.str().c_str(), response.str().size());
//	return (response.str());
//}

void	request::end()
{
	if (_fd > 0)
	{
		epollMood(_fd, EPOLL_CTL_DEL);
		close(_fd);
	}
	// If there is a running CGI child, terminate it before closing the pipe
	if (_cgiChild > 0)
	{
		kill(_cgiChild, SIGKILL);
		waitpid(_cgiChild, NULL, 0);
		_cgiChild = 0;
	}
	if (_infile > 0)
		close(_infile);

	requestHandler::delReq(_fd);
}

const std::string& request::getContentType()	const { return _contentType;	}
const std::string& request::getBody()			const { return _body;			}
const std::string& request::getMethod()			const { return _method;			}