#pragma once

#include "WebServer.hpp"

class requestHandler
{
	private:
		static std::map<int, request*> 	_requests;

		requestHandler();
		~requestHandler();

		static request*&	getReq(int fd);
		
	public:
		static void			readReq(int fd, std::map <int, serverConfig*>& server);
		static void			delReq(int fd);
};

