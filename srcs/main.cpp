#include "WebServer.hpp"
#include "conf.hpp"
#include "request.hpp"
#include "methods.hpp"
#include "resp.hpp"

#define MAX_EVENTS 5

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

	setNonBlocking(epfd);

	struct epoll_event	events[MAX_EVENTS];

	int running = 1;
	while (running)
	{
		//printf("Polling for input...\n");
		int n = epoll_wait(epfd, events, MAX_EVENTS, 0);
		for (int i = 0; i < n; i++)
		{
			int fd = events[i].data.fd;

			if (fd == conf::server())
			{
				myAccept(epfd);
			} 
			else
			{
				request::readReq(fd);
			}
		}
		usleep(100000);
	}
}

/*
			if (fd == conf::server()) //new request incoming
			{
				//myAccept();
				setNonBlocking(fd);
				while (1) {
					int client_fd = accept(conf::server(), NULL, NULL);
					if (client_fd == -1) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							// No more clients to accept right now
							break;
						} else {
							perror("accept");
							break;
						}
					}
					// register client_fd in epoll, etc...
				}
				ev.events = EPOLLIN;
				ev.data.fd = fd;
				if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
					perror("epoll_ctl: client_fd");
					close(fd);
				}
				request::addResp(fd);
			}
			else
			{
				request::addResp(fd);
				write(1, "333 ", 4);
				resp* temp = request::getResp(fd);
				if (temp)
				{
					try
					{
						temp->readSocket();
						if (temp->finished())
						{
							temp->doTheThing();
							request::delResp(fd);
							epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
						}
					}
					catch(const std::exception& e)
					{
						//Handle the errors
						std::cerr << e.what() << '\n';
					}
				}
			}
*/