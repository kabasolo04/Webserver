#pragma once

#include "WebServer.hpp"

class request
{
	private:
		static std::map<int, response*> 	_responses;

		request();
		~request();

		static void	delResp(int fd);
		static void	addResp(int fd);

	public:
		static void	readReq(int fd);
};

