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

std::string readLine(int fd)
{
	std::string line;
	char c;

	while (true) // Read until CRLF
	{
		ssize_t n = read(fd, &c, 1);
		if (n < 0)
			throw httpResponse(INTERNAL_SERVER_ERROR);
		if (n == 0)
			throw std::runtime_error("Client Disconnected");

		line += c;

		if (line.size() >= 2 && line[line.size() - 2] == '\r' && line[line.size() - 1] == '\n')
		{
			line.erase(line.end() - 2, line.end()); // Remove the CRLF
			break;
		}
	}
	return line;
}

request* createMethod(int fd, serverConfig& server)
{
	std::istringstream iss(readLine(fd)); // Split by spaces
	std::string method, path, protocol;

	if (!(iss >> method >> path >> protocol))
		throw httpResponse(BAD_REQUEST);

	if (protocol != "HTTP/1.0" && protocol != "HTTP/1.1")
		throw httpResponse(LOL);

	location&  temp = server.getLocation(path);

	if (!temp.methodAllowed(method))
		throw httpResponse(METHOD_NOT_ALLOWED);

	if (method == "GET")
		return new myGet(fd, path, temp);
	if (method == "POST")
		return new myPost(fd, path, temp);
	if (method == "DELETE")
		return new myDelete(fd, path, temp);

	throw httpResponse(METHOD_NOT_ALLOWED);
}

request*	requestHandler::getReq(int fd, serverConfig& server)
{
	std::map<int, request*>::iterator it = _requests.find(fd);

	if (it == _requests.end())
	{
		delReq(fd);
		_requests[fd] = createMethod(fd, server);
	}
	return (_requests[fd]);
}

void	requestHandler::readReq(int fd, serverConfig& server)
{
	try { return getReq(fd, server)->exec(); }

	catch(const httpResponse& e)
	{
		e.sendResponse(fd);  // Sends the html for all methods and Errors
	}
	catch(const std::exception &e)
	{
		std::cout << e.what() << std::endl; // Catch for strange errors
	}

	delReq(fd); // Once finished clean it
}
