#include "WebServer.hpp"
#include <csignal>

#define MAX_EVENTS 100

void signalHandler(int sig)
{
	std::cout << "Interrupt handle " << sig << std::endl;
	requestHandler::freeAll();
	exit(0);
}

void myAccept(int fd, serverConfig& server)
{
	while (true)
	{
		struct sockaddr_in client_addr;
		memset(&client_addr, 0, sizeof(client_addr));
		socklen_t client_len = sizeof(client_addr);
		int clientFd = accept(fd, (sockaddr*)&client_addr, &client_len);
		if (clientFd == -1)
		{
			if (!(errno == EAGAIN || errno == EWOULDBLOCK))
				std::cout << "Error: Accept | main.cpp - myAccept()" << std::endl;
			break;
		}
		requestHandler::addReq(clientFd, server);
	}
}

bool	newRequest(int fd)
{
	std::vector<serverConfig>::iterator serverIt = conf::serverBegin();

	for (; serverIt != conf::serverEnd(); ++serverIt)
	{
		serverConfig& server = *serverIt;
		std::vector<listenEntry>::iterator listenIt = server.listenBegin();
	
		for (; listenIt != server.listenEnd(); ++listenIt)
			if (listenIt->_fd == fd)
				return (myAccept(fd, server), true);
	}
	return false;
}

int main(int argc, char **argv)
{
	if (argc != 2)
		return (std::cerr << "[ERROR]: './webserv /config/file/path' | main.cpp - main()" << std::endl, 1);

	try { conf::parseFile(argv[1]); } catch(const std::exception &e)
	{
		return (std::cout << "[ERROR]: " << e.what() << std::endl, 1);
	}

	signal(SIGINT, signalHandler);
	signal(SIGPIPE, SIG_IGN);

	struct epoll_event	events[MAX_EVENTS];

	while (1)
	{
		int n = epoll_wait(conf::epfd(), events, MAX_EVENTS, -1);

		if (n < 0)
			return (std::cout << "Error: epoll_wait failed | main.cpp - main()" << std::endl, 1);
		
		for (int i = 0; i < n; i++)
			if (!newRequest(events[i].data.fd))
				requestHandler::execReq(events[i].data.fd);
	}
}
