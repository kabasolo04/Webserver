#pragma once

#include "WebServer.hpp"

class requestHandler
{
	private:
		static std::map<int, request*> 	_requests;

		requestHandler();
		~requestHandler();
		
	public:
		static void			addReq(int fd, serverConfig& server);
		static void			execReq(int fd);
		static void			delReq(int fd);
};
