#pragma once

#include "WebServer.hpp"

class conf
{
	protected:
		static std::vector<serverConfig>	_servers;
		static int							_epfd;	// Global epoll file descriptor
		static epoll_event					_event;	// Global shared event struct

		conf();
		~conf();
	
	public:
		static std::vector<serverConfig>::iterator	serverBegin();
		static std::vector<serverConfig>::iterator	serverEnd();

		static const int&			epfd();
		static const epoll_event&	event();

		static void	parseFile(std::string filename);
};
