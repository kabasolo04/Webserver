#pragma once

#include "WebServer.hpp"

class myGet: public request
{
	private:
		void			setQuery();
		void			generateAutoIndex();

	public:
		myGet(int fd, std::string target, location& loc, std::string reqLine);
		~myGet();
		void			process();
};

class myPost: public request
{
	private:
		bool	chunkedCheck();
		void	handleMultipart();
		bool	readBody();


	public:
		myPost(int fd, std::string target, location& loc, std::string reqLine);
		~myPost();
		void		process();
};

class myDelete: public request
{
	public:
		myDelete(int fd, std::string target, location& loc, std::string reqLine);
		~myDelete();
		void		process();
};
