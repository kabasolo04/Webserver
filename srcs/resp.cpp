#include "resp.hpp"

resp::resp() {}
resp::resp(int fd)
{
	_fd = fd;
	_finished = 0;
}

resp::~resp() {}

void	resp::readFd()
{
	char buffer[100];

	ssize_t len = read(_fd, buffer, sizeof(buffer));

	if (len > 0)
	{
		_buffer.append(buffer, len);
		//std::cout << _buffer << std::endl;
	}
}

void	resp::readSocket()
{
	readFd();
	if (makeTheCheck(_buffer))
		_finished = 1;
}

bool	resp::finished()
{
	return (_finished);
}