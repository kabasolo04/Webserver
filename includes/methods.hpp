#pragma once

#include "WebServer.hpp"

class myGet: public request
{
	private:
		void			setQuery();
		void			generateAutoIndex();

	public:
		myGet(request* baby);
		~myGet();
		StatusCode	setUpMethod();
		StatusCode	processMethod();
};

class myPost: public request
{
	private:
		//bool	chunkedCheck();
		StatusCode	handleMultipart();

	public:
		myPost(request* baby);
		//myPost(int fd, std::string target, location& loc);
		~myPost();
		StatusCode	setUpMethod();
		StatusCode	processMethod();
};

class myDelete: public request
{
	public:
		myDelete(request* baby);
		//myDelete(int fd, std::string target, location& loc);
		~myDelete();
		StatusCode	setUpMethod();
		StatusCode	processMethod();
};
