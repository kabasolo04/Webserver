#include "request.hpp"
#include "methods.hpp"
#include "conf.hpp"

std::map<int, resp*> request::_responses;

void	request::delResp(int fd)
{
	std::map<int, resp*>::iterator it = _responses.find(fd);
	if (it != _responses.end())
	{
		delete it->second;   // important!
		close(fd);
		_responses.erase(it);
	}
}

void	request::addResp(int fd)
{
	delResp(fd);
	setNonBlocking(fd);
	_responses[fd] = new get(fd);
}

void	request::readReq(int fd)
{
	std::map<int, resp*>::iterator it = _responses.find(fd);
	if (it == _responses.end())
	{
		addResp(fd);
		it = _responses.find(fd);
	}
	it->second->readSocket();
	if (it->second->finished())
	{
		it->second->doTheThing();
		delResp(fd);
	}
}