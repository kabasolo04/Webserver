#include "WebServer.hpp"

#define FINISHED 1
#define NONE 0

std::map<int, request*> requestHandler::_requests;

void	requestHandler::delReq(int fd)
{
	std::map<int, request*>::iterator it = _requests.find(fd);
	if (it != _requests.end())
	{
		delete it->second;
		epoll_ctl(fd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		_requests.erase(it);
	}
}
/*
request*	createMethod(int fd, serverConfig& server)
{
	char buffer[7];
	ssize_t len = read(fd, buffer, sizeof(buffer));

	if (len < 0)
		throw httpResponse(INTERNAL_SERVER_ERROR);
	if (len == 0)
		throw std::runtime_error("Client Disconnected");

	std::string raw(buffer);
	size_t pos = raw.find(' ');

	if (pos == std::string::npos)
		throw httpResponse(BAD_REQUEST);

	std::string method = raw.substr(0, pos);

//	if (!conf::methodAllowed(method))
//		throw httpResponse(METHOD_NOT_ALLOWED);
	if (method == "GET")
		return new myGet(fd, raw, server);
	if (method == "POST")
		return new myPost(fd, raw, server);
	if (method == "DELETE")
		return new myDelete(fd, raw, server);
	throw httpResponse(METHOD_NOT_ALLOWED);
}
*/

request*&	requestHandler::getReq(int fd)
{
	std::map<int, request*>::iterator it = _requests.find(fd);

	if (it == _requests.end())
	{
		delReq(fd);
		_requests[fd] = new request(fd);
	}
	return (_requests[fd]);
}

void	requestHandler::readReq(int fd, std::map <int, serverConfig*>& servers)
{
	request* req;

	try
	{
		req = getReq(fd);

		if (req->readSocket() != FINISHED) return; // Goes out only if the request hasnt been fully readed or an error ocurred

		if (req->methodSelected() == NONE) req = req->selectMethod(servers); // Turns into the asked method by the requests header

		return req->process();	// Each method does its thing
	}

	catch(const httpResponse& e)
	{
		e.sendResponse(fd);  // Sends the html for all methods and Errors
	}

	catch(const std::exception &e)
	{
		std::cout << "Error: " << e.what() << std::endl; // Catch for strange errors
	}

	delReq(fd); // Once finished clean it
}
