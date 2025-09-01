#include "WebServer.hpp"

std::map<int, response*> request::_responses;

void	request::delResp(int fd)
{
	std::map<int, response*>::iterator it = _responses.find(fd);
	if (it != _responses.end())
	{
		delete it->second;   // important!
		close(fd);
		_responses.erase(it);
	}
}

/*
response*	createMethod(int fd)
{
	Markel haz lo tuyo warro
}
*/

void	request::addResp(int fd)
{
	delResp(fd);
	//_responses[fd] = createMethod(fd);
	_responses[fd] = new myGet(fd, "Mujejeej");
}

void	request::readReq(int fd)
{
	std::map<int, response*>::iterator it = _responses.find(fd);
	if (it == _responses.end())
	{
		addResp(fd);
		it = _responses.find(fd);
	}
	try
	{
		it->second->readSocket();
		if (it->second->finished())
		{
			it->second->doTheThing();
			delResp(fd);
		}
	}
	catch(const std::exception& e)
	{
		//generation of errors
		//error::createResponse(it->second);
		//delResp()
		std::cerr << e.what() << '\n';
	}
}
