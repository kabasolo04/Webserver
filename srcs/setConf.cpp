#include "setConf.hpp"

void	setConf::setServer()
{
	int listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenFd < 0)
		throw std::runtime_error("Socket failed | setConf.cpp - setServer()");

	int yes = 1;
	if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
	{
		close(listenFd);
		throw std::runtime_error("Setsockopt failed | setConf.cpp - setServer()");
	}

	struct sockaddr_in addr;
	//std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY; // bind to all interfaces
	addr.sin_port = htons(_port);

	if (bind(listenFd, (sockaddr*)&addr, sizeof(addr)) < 0)
	{
		close(listenFd);
		throw std::runtime_error("Bind failed | setConf.cpp - setServer()");
	}

	if (listen(listenFd, SOMAXCONN) < 0)
	{
		close(listenFd);
		throw std::runtime_error("Listen failed | setConf.cpp - setServer()");
	}
	_server = listenFd;

	setNonBlocking(_server);
}

void	setConf::parseFile(std::string filename)
{
	

	_methods.push_back("GET");
	_methods.push_back("POST");
	_methods.push_back("DELETE");

	(void)filename;
	//Configuration file parsing

}

void	setConf::setEpoll()
{
	_epfd = epoll_create1(0);
	if (_epfd == -1)
		throw std::runtime_error("epoll_create | setConf.cpp - setEpoll()");

	_event.events = EPOLLIN;	//only watching for input events
	_event.data.fd = _server;	//the fd its gonna watch

	if (epoll_ctl(_epfd, EPOLL_CTL_ADD, _server, &_event) == -1)
		throw std::runtime_error("epoll_ctl | setConf.cpp - setEpoll()");
}
