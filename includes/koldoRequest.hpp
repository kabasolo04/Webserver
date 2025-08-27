#pragma once
#include "WebServer.hpp"

class Iresp;

class koldoRequest
{
	private:
		std::map<int, Iresp*>  _responses;

	public:
		
		koldoRequest();
		~koldoRequest();

		void	readSockets();
		void	sendResponses();
};

