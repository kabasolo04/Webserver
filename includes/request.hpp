#pragma once

#include "WebServer.hpp"

#define BUFFER 1

enum nodes
{
	READ_BODY,
	READ_CHUNKED,
	PROCESS,
	STATE_COUNT // always last, to know how many states exist
};

class request
{
	protected:
		int									_fd;
		std::string							_method;
		std::string							_buffer;
		std::string							_path;
		std::string							_protocol;
		std::map<std::string, std::string>	_headers;
		std::string							_body;
		std::string							_contentType;
		std::string							_query;
		
		//std::string&						_target;
		location&							_location;
		
		void (request::*_function)();

		//bool	readUntil(std::string& eof);
		//bool	readUntil(size_t size);

		//void		setReqLineVars();
		void		setHeaderVars();
		void		printHeaders();

		void			readHeader();
		void			readBody();
		void			readChunked();
		virtual void	process() = 0;

		void	cgi(std::string command);

		void	nextNode(nodes node);
	
		request(int fd, std::string target, location& loc);
		
	public:
		virtual ~request();

		request& operator = (const request& other);
	
		const std::string& getContentType() const;
		const std::string& getBody() const;
		const std::string& getMethod() const;
		const std::string& getPath() const;
		const std::string& getQuery() const;

		void	setBody(std::string body);
		void	setContentType(std::string contentType);

		void	exec();
};
