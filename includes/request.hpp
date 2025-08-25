#pragma once
#include <sys/epoll.h>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <map>
#include <cstring>
#include <cstdio>
#include <cerrno>

class request
{
	private:
		int									_ep;
		int									_server;
		int									_currentFd;
		epoll_event							_ev;
		std::vector <struct epoll_event>	_events; //dinamic size event in case lots of users log into the web
		std::map<int, std::string>			_buffers;
		std::string							_request;


		request();

	public:
		request(int server);
		~request();

		std::string		listen();
		void		readIt(int fd);

		const std::string&	getRequest();
		int					getEventFd();
		int			request_status;
};
