#pragma once

#include "resp.hpp"

class get: public resp
{
	private:
		get();
	public:
		get(int fd);
		~get();
		void	doTheThing();
		bool	makeTheCheck(std::string buffer);
};

/*

class Post: public Iresp
{
	public:
		Post();
		~Post();
		void	readSocket();
		void	doTheThing();
};

class Delete: public Iresp
{
	public:
		Delete();
		~Delete();
		void	readSocket();
		void	doTheThing();
};

*/
