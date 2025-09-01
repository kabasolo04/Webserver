#pragma once

#include "WebServer.hpp"

class myGet: public response
{
	private:
		myGet();
	
	public:
		myGet(int fd, std::string buffer);
		~myGet();
		void	doTheThing();
		bool	makeTheCheck();
};

class myPost: public response
{
	private:
		myPost();

	public:
		myPost(int fd, std::string buffer);
		~myPost();
		void	doTheThing();
		bool	makeTheCheck();
};

class myDelete: public response
{
	private:
		myDelete();
	
	public:
		myDelete(int fd, std::string buffer);
		~myDelete();
		void	doTheThing();
		bool	makeTheCheck();
};
