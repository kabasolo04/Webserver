#include "WebServer.hpp"

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
				perror("accept");
				break;
			}
		}
		setNonBlocking(client_fd);
		struct epoll_event client_event;
		memset(&client_event, 0, sizeof(client_event));
		client_event.data.fd = client_fd;
		client_event.events = EPOLLIN;
		epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_event);
	}
}

int main()
{
//	if (argc != 2)
//		return (0);
//	try {
		conf::setConfig("Filename");
//	}

	//char read_buffer[10 + 1];
	
	int epfd = epoll_create1(0);
	if (epfd == -1) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	struct epoll_event	event;

	event.events = EPOLLIN;			//only watching for input events
	event.data.fd = conf::server();	//the fd its gonna watch

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, conf::server(), &event) == -1) {
		perror("epoll_ctl: server_fd");
		exit(EXIT_FAILURE);
	}

//	setNonBlocking(epfd);

	struct epoll_event	events[5];

	while (1)
	{
		int n = epoll_wait(epfd, events, 5, 0);
		for (int i = 0; i < n; i++)
		{
			int fd = events[i].data.fd;

			if (fd == conf::server())
				myAccept(epfd);
			else
				requestHandler::readReq(fd);
		}
		usleep(100000);
	}
}
