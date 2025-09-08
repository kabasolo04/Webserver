#pragma once

#include "WebServer.hpp"

class myGet: public request
{
	private:
		myGet();
	
	public:
		myGet(int fd, std::string buffer);
		~myGet();
		void		brain(const std::string& status_n_msg, std::ifstream& file);
		void		doTheThing();
		bool		makeTheCheck();
};

class myPost: public request
{
	private:
		bool	_headerCheck;

		myPost();

	public:
		myPost(int fd, std::string buffer);
		~myPost();
		void		doTheThing();
		bool		makeTheCheck();
};

class myDelete: public request
{
	private:
		myDelete();
	
	public:
		myDelete(int fd, std::string buffer);
		~myDelete();
		void		doTheThing();
		bool		makeTheCheck();
};
