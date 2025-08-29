#pragma once

#include "WebServer.hpp"

class resp //Interface
{
	protected:
		int 			_fd;
		
	private:
		std::string		_buffer;
		bool			_finished;

		
		void	readFd();
		resp();

	public:
		resp(int fd);
		virtual ~resp();
		
		virtual void	doTheThing() = 0;
		virtual bool	makeTheCheck(std::string buffer) = 0;
		
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