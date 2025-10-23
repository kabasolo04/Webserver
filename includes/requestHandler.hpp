#pragma once

#include "WebServer.hpp"

class requestHandler
{
	private:
		static std::map<int, request*> 	_requests;

		requestHandler();
		~requestHandler();

		static request*	getReq(int fd, serverConfig& server);
		
	public:
		static void			readReq(int fd, serverConfig& server);
		static void			delReq(int fd);
};

