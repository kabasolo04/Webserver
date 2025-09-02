#include "WebServer.hpp"

request::request() {}

request::request(int fd, std::string buffer): _fd(fd), _buffer(buffer), _finished(0) {}

request::~request() {}

void	request::readFd()
{
	char buffer[100];

	ssize_t len = read(_fd, buffer, sizeof(buffer));

	if (len > 0)
	{
		_buffer.append(buffer, len);
		//std::cout << _buffer << std::endl;
	}
}

void	request::readSocket()
{
	readFd();
	if (makeTheCheck())
		_finished = 1;
}

bool	request::finished()
{
	return (_finished);
}
