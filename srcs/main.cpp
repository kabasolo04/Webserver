#include "WebServer.hpp"
#include "setConf.hpp"

void myAccept(int epfd)
{
	while (true)
	{
		struct sockaddr_in client_addr;
		memset(&client_addr, 0, sizeof(client_addr));
		socklen_t client_len = sizeof(client_addr);
		int client_fd = accept(conf::server(), (sockaddr*)&client_addr, &client_len);
		if (client_fd == -1)
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
		struct epoll_event client_event;
		memset(&client_event, 0, sizeof(client_event));
		client_event.data.fd = client_fd;
		client_event.events = EPOLLIN;
		epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_event);
	}
}

int main(int argc, char **argv, char **envp)
{
	g_envp = envp;
	if (argc != 2)
		return (std::cerr << "Error: './webserv /config/file/path' | main.cpp - main()" << std::endl, 0);

	try
	{
		setConf::parseFile(argv[1]);
		setConf::setServer();
		setConf::setEpoll();
	}
	catch(const std::exception &e)
	{
		std::cout << "Error: " << e.what() << std::endl;
		return (1);
	}

	struct epoll_event	events[5];

	while (1)
	{
		int n = epoll_wait(conf::epfd(), events, 5, -1);
		for (int i = 0; i < n; i++)
		{
			int fd = events[i].data.fd;

			if (fd == conf::server())
				myAccept(conf::epfd());
			else
				requestHandler::readReq(fd);
		}
	}
}
