#pragma once

#include "WebServer.hpp"

#define BUFFER 1

class request
{
	protected:
		int									_fd;
		std::string							_buffer;
		std::string							_path;
		std::string							_protocol;
		std::map<std::string, std::string>	_headers;
		std::string							_body;
		std::string							_contentType;

		request(int fd, std::string buffer);
		virtual bool	check() = 0;
		
		void		setReqLineVars();
		void		setHeaderVars();
		void		printHeaders();
		
		private:
		request();
		
	public:
		virtual ~request();
		
		bool			readSocket();
		virtual void	process() = 0;
	
		const std::string& getContentType() const;
		const std::string& getBody() const;
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
