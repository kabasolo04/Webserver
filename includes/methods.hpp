#pragma once

#include "WebServer.hpp"

class myGet: public request
{
	private:
		myGet();
	
	public:
		myGet(int fd, std::string buffer);
		~myGet();
		void		response(std::ifstream &file);
		void		process();
		bool		check();
};

class myPost: public request
{
	private:
		bool	_headerCheck;

		myPost();

	public:
		myPost(int fd, std::string buffer);
		~myPost();
		void		process();
		bool		check();
		bool		chunkedCheck();
		void		handleMultipart();
};

class myDelete: public request
{
	private:
		myDelete();
	
	public:
		myDelete(int fd, std::string buffer);
		~myDelete();
		void		process();
		bool		check();
};
