#include "WebServer.hpp"

#define MAX_EVENTS 10

void myAccept(std::map <int, serverConfig*>& serverMap, int portFd, serverConfig& server)
{
	while (true)
	{
		struct sockaddr_in client_addr;
		memset(&client_addr, 0, sizeof(client_addr));
		socklen_t client_len = sizeof(client_addr);
		int clientFd = accept(portFd, (sockaddr*)&client_addr, &client_len);
		if (clientFd == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				break; // no more clients
			}
			else
			{
				std::cout << "Error: Accept | main.cpp - myAccept()" << std::endl;
				//perror("accept"); // something went wrong but we continue, this is a extrange error
				break;
			}
		}
		setNonBlocking(clientFd);
		struct epoll_event client_event;
		memset(&client_event, 0, sizeof(client_event));
		client_event.data.fd = clientFd;
		client_event.events = EPOLLIN;
		epoll_ctl(conf::epfd(), EPOLL_CTL_ADD, clientFd, &client_event);
		serverMap[clientFd] = &server;
	}
}

bool	newRequest(int fd, std::map <int, serverConfig*>& serverMap)
{
	std::vector<serverConfig>::iterator serverIt = conf::serverBegin();

	for (; serverIt != conf::serverEnd(); ++serverIt)
	{
		serverConfig& server = *serverIt;
		std::vector<listenEntry>::iterator listenIt = server.listenBegin();
	
		for (; listenIt != server.listenEnd(); ++listenIt)
			if (listenIt->_fd == fd)
				return (myAccept(serverMap, fd, server), true);
	}

	return false;
}

int main(int argc, char **argv)
{
	if (argc != 2)
		return (std::cerr << "[ERROR]: './webserv /config/file/path' | main.cpp - main()" << std::endl, 1);
	try
	{
		conf::parseFile(argv[1]);
	}
	catch(const std::exception &e)
	{
		return (std::cout << "[ERROR]: " << e.what() << std::endl, 1);
	}

	struct epoll_event	events[MAX_EVENTS];
	std::map <int, serverConfig*> serverMap;

	while (1)
	{
		int n = epoll_wait(conf::epfd(), events, MAX_EVENTS, -1);
	
		if (n < 0)
			return (std::cout << "Error: epoll_wait failed | main.cpp - main()" << std::endl, 1);

		for (int i = 0; i < n; i++)
		{
			if (!newRequest(events[i].data.fd, serverMap))
			{
				requestHandler::readReq(events[i].data.fd, *serverMap[events[i].data.fd]);
			}

		}
	}
}
