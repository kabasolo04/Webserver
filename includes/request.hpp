#pragma once

#include "WebServer.hpp"

#define BUFFER 1

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
		
		location							_server;

		bool								_methodSelected;
		
		void		setReqLineVars();
		void		setHeaderVars();
		void		printHeaders();
	
		request(request& other, std::map <int, serverConfig*>& servers);

	public:
		request(int fd);
		virtual ~request();

		request& operator = (const request& other);

		bool			readSocket();
		virtual void	process();

		request*		selectMethod(std::map <int, serverConfig*>& servers);
		bool			methodSelected();
	
		const std::string& getContentType() const;
		const std::string& getBody() const;
		const std::string& getMethod() const;
		const std::string& getPath() const;
		const std::string& getQuery() const;

	
		void	setBody(std::string body);
		void	setContentType(std::string contentType);
		void	cgi(std::string command);
};
