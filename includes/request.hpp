#pragma once

#include "WebServer.hpp"

class resp;

class request
{
	private:
		static std::map<int, resp*> 	_responses;

		request();
		~request();

		static void	delResp(int fd);
		static void	addResp(int fd);

	public:
		static void	readReq(int fd);
};

