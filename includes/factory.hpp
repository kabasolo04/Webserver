#pragma once

#include "WebServer.hpp"

class Afactory
{
	private:
		Afactory();
		~Afactory();

	public:
		static Iresp* decideMethod(std::string firsLine);
};

Iresp* Afactory::decideMethod(std::string firsLine)
{
	if (firsLine.compare(0, 4, "GET ") == 0)
		return new Get(firstLine);
	/*
	if (firsLine.compare(0, 5, "POST ") == 0)
		return new Post();
	if (firsLine.compare(0, 7, "DELETE ") == 0)
		return new Delete();
	*/
	return NULL;
}