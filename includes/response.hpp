#pragma once

#include "WebServer.hpp"

class response
{
	protected:
		int 			_fd;
		std::string		_buffer;
		
	private:
		bool			_finished;

		void	readFd();
		response();

	public:
		response(int fd, std::string buffer);
		virtual ~response();
		
		virtual void	doTheThing() = 0;
		virtual bool	makeTheCheck() = 0;
		
		void	readSocket();
		bool	finished();
};
	
	/*
	std::string		_header;
	std::string		_body;

	std::string		_method;
	std::string		_path;
	std::string		_http_version;
	unsigned int	_status;
	bool			_allowed;

	struct epoll_event	_event;
	*/