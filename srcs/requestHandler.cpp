#include "WebServer.hpp"

std::map<int, request*> requestHandler::_requests;

void	requestHandler::delResp(int fd)
{
	std::map<int, request*>::iterator it = _requests.find(fd);
	if (it != _requests.end())
	{
		delete it->second;   // important!
		close(fd);
		_requests.erase(it);
	}
}

request*	createMethod(int fd)
{
	char buffer[7];
	ssize_t len = read(fd, buffer, sizeof(buffer));

	if (len <= 0)
		throw std::runtime_error("Null request");
	std::string raw(buffer);
	size_t pos = raw.find(' ');
	if (pos == std::string::npos)
		throw std::runtime_error("Invalid request: no space found");

	std::string method = raw.substr(0, pos);
	std::string rest = raw.substr(pos + 1); // everything after the space
	if (!conf::methodAllowed(method))
		throw std::runtime_error("Unavailable method: " + method);
	if (method == "GET")
		return new myGet(fd, rest);
	if (method == "POST")
		return new myPost(fd, rest);
	if (method == "DELETE")
		return new myDelete(fd, rest);
	throw std::runtime_error("Invalid request");
}

void	requestHandler::addResp(int fd)
{
	delResp(fd);
	_requests[fd] = createMethod(fd);
	//_requests[fd] = new myGet(fd, "Mujejeej");
}

void	requestHandler::readReq(int fd)
{
	std::map<int, request*>::iterator it = _requests.find(fd);
	if (it == _requests.end())
	{
		addResp(fd);
		it = _requests.find(fd);
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
