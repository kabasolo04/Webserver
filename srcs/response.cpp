#include "WebServer.hpp"

response::response() {}

response::response(int fd, std::string buffer): _fd(fd), _buffer(buffer), _finished(0) {}

response::~response() {}

void	response::readFd()
{
	char buffer[100];

	ssize_t len = read(_fd, buffer, sizeof(buffer));

	if (len > 0)
	{
		_buffer.append(buffer, len);
		//std::cout << _buffer << std::endl;
	}
}

void	response::readSocket()
{
	readFd();
	if (makeTheCheck())
		_finished = 1;
}

bool	response::finished()
{
	return (_finished);
}
