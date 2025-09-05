#pragma once

#include "WebServer.hpp"

class requestHandler
{
	private:
		static std::map<int, request*> 	_requests;

		requestHandler();
		~requestHandler();

		static void	delReq(int fd);
		static void	addReq(int fd);

	public:
		static void	readReq(int fd);
};

