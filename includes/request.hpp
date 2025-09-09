#pragma once

#include "WebServer.hpp"

#define BUFFER 4096

class request
{
	protected:
		int									_fd;
		std::string							_buffer;
		std::string							_path;
		std::string							_protocol;
		std::map<std::string, std::string>	_headers;

		int									_status;
		std::string							_errorMessage;
		
	private:
		bool			_finished;

		void	readFd();
		request();

	public:
		request(int fd, std::string buffer);
		virtual ~request();
		
		virtual void	doTheThing() = 0;
		/* virtual void	brain(const std::string& status_n_msg, std::ifstream& file) = 0; */
		virtual bool	makeTheCheck() = 0;
		
		void		readSocket();
		bool		finished();
		void		getReqLineVars();
		void		getHeaderVars();
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
