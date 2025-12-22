#pragma once

#include "WebServer.hpp"

class requestHandler
{
	private:
		static std::map<int, request*>	_requests;
		static std::map<int, bool>		_cgi;

		requestHandler();
		~requestHandler();
		
	public:
		static void	addReq(int fd, serverConfig& server);
		static void	execReq(int fd);
		static void	delReq(int fd);

		static void	addCgi(int fd);
		static bool	getCgi(int fd);
		static void	setCgi(int fd, bool mood);
		static void	delCgi(int fd);
};
