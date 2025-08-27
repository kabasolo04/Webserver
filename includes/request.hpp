#pragma once
#include "WebServer.hpp"

class request
{
	private:
		int									_ep;
		int									_server;
		int									_currentFd;
		epoll_event							_ev;
		std::vector <struct epoll_event>	_events; //dinamic size event in case lots of users log into the web
		std::map<int, std::string>			_buffers;


		request();
		void	myAccept(int i);
		void	readIt(int fd);

	public:
		request(int server);
		~request();

		void		listen();
};
