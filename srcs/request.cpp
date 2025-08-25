#include "request.hpp"

request::request(int server): _server(server), _events(128)
{
	_ep = epoll_create1(0);
	_ev.events = EPOLLIN;
	_ev.data.fd = _server;
	epoll_ctl(_ep, EPOLL_CTL_ADD, _server, &_ev);
}

request::~request()
{
	close(_ep);
}
void request::readIt(int fd)
{
    char buffer[512];

    ssize_t len = read(fd, buffer, sizeof(buffer));

	if (len > 0)
		_buffers[fd].append(buffer, len);
	
}

bool	isRequestComplete(const std::string &buf)
{
	//write(1, buf.c_str(), buf.size());
    return buf.find("\r\n\r\n") != std::string::npos;
}

std::string request::listen()
{
	int n = epoll_wait(_ep, _events.data(), _events.size(), -1);

	//if (n == (int)_events.size())
    //    _events.resize(_events.size() * 2);

	write(1, "Fuk ", 4);
	for (int i = 0; i < n; i++)
	{
		int fd = _events[i].data.fd;

		if (fd == _server) {
			// New incoming connections waiting
			while (true) {
				int client = accept(_server, NULL, NULL);
				if (client == -1) {
					if (errno == EAGAIN || errno == EWOULDBLOCK) break; // No more pending
					perror("accept");
					break;
				}
				fcntl(client, F_SETFL, O_NONBLOCK);
				int flags = fcntl(fd, F_GETFL, 0);
				/*
				if (flags & O_NONBLOCK) {
					std::cout << "Socket is non-blocking\n";
				} else {
					std::cout << "Socket is blocking\n";
				}
				*/
				_events[i].events = EPOLLIN;
				_events[i].data.fd = client;
				epoll_ctl(_ep, EPOLL_CTL_ADD, client, &_events[i]);
			}
		}
		else
		{
			// Existing client has data to read
			readIt(fd);
			if (isRequestComplete(_buffers[fd]))
			{
				write(1, _buffers[_events[i].data.fd].c_str(), _buffers[_events[i].data.fd].size());
				close(_events[i].data.fd);
				_buffers[_events[i].data.fd].clear();
			}
		}
	}
	return ("");
}

const std::string&	request::getRequest()
{
	return (_request);
}

int	request::getEventFd()
{
	return (_currentFd);
}
