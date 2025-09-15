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
		static void			readReq(int fd);
		static void			delReq(int fd);
};

