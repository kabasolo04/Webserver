#pragma once

#include "WebServer.hpp"

class myGet: public request
{
	private:
		myGet();
	
	public:
		myGet(request* req, std::map <int, serverConfig*>& servers);
		~myGet();
		void			response(std::ifstream &file);
		void			setQuery();
		void			process();
		bool			check();
		void			generateAutoIndex();
};

class myPost: public request
{
	private:
		myPost();
		bool	chunkedCheck();
		void	handleMultipart();
		void	saveFile(const std::string &part);
		bool	readBody();


	public:
		myPost(request* req, std::map <int, serverConfig*>& servers);
		~myPost();
		void		process();
};

class myDelete: public request
{
	private:
		myDelete();
	
	public:
		myDelete(request* req, std::map <int, serverConfig*>& servers);
		~myDelete();
		void		process();
};
