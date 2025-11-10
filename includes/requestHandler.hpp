#pragma once

#include "WebServer.hpp"

class requestHandler
{
	private:
		static std::map<int, request*> 	_requests;

		requestHandler();
		~requestHandler();

	public:
		static void	delReq(int fd);
		static void	execReq(int fd);
		static bool	transform(int fd, request* baby);
		static void	addReq(int fd, serverConfig& server);
};
