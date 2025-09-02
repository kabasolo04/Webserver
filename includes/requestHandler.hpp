#pragma once

#include "WebServer.hpp"

class requestHandler
{
	private:
		static std::map<int, request*> 	_requests;

		requestHandler();
		~requestHandler();

		static void	delResp(int fd);
		static void	addResp(int fd);

	public:
		static void	readReq(int fd);
};

