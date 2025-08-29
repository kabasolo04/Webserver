#include "request.hpp"

Iresp::Iresp() {
    // Optional: leave empty if there's nothing to initialize
}

Iresp::~Iresp() {}

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
    return buf.find("\r\n\r\n") != std::string::npos;
}

void request::myAccept(int i)
{

	while (true)
	{
		int client = accept(_server, NULL, NULL);

		if (client == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK) break; // No more pending
			perror("accept");
			break;
		}

		fcntl(client, F_SETFL, O_NONBLOCK); // Making it non-blocking
		int flags = fcntl(_events[i].data.fd, F_GETFL, 0);
		(void)flags;

		_events[i].events = EPOLLIN;
		_events[i].data.fd = client;
		epoll_ctl(_ep, EPOLL_CTL_ADD, client, &_events[i]);
	}
}

void	request::listen()
{
	int n = epoll_wait(_ep, _events.data(), _events.size(), -1);

	if (n == (int)_events.size())
        _events.resize(_events.size() * 2);

	for (int i = 0; i < n; i++)
	{
		int fd = _events[i].data.fd;

		if (fd == _server)
		{
			myAccept(i); // New incoming connections waiting
		}
		else
		{
			readIt(fd);
			if (isRequestComplete(_buffers[fd]))
			{
				write(1, _buffers[fd].c_str(), _buffers[fd].size());
				close(fd);
				_buffers[fd].clear();
				throw std::runtime_error("Request completed");
			}
		}
	}
}

class myClass
{
	private:

		static std::string name;

		myClass();
		~myClass();

	public:

		void				setName();
		const std::string&	getName();
};