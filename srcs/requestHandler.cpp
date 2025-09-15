#include "WebServer.hpp"

#define FINISHED 1

std::map<int, request*> requestHandler::_requests;

void	requestHandler::delReq(int fd)
{
	std::map<int, request*>::iterator it = _requests.find(fd);
	if (it != _requests.end())
	{
		delete it->second;   // important!
		epoll_ctl(fd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		_requests.erase(it);
	}
}

request*	createMethod(int fd)
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

	if (!conf::methodAllowed(method))
		throw httpResponse(METHOD_NOT_ALLOWED);
	if (method == "GET")
		return new myGet(fd, raw);
	if (method == "POST")
		return new myPost(fd, raw);
	if (method == "DELETE")
		return new myDelete(fd, raw);

	throw httpResponse(METHOD_NOT_ALLOWED);
}

request*&	requestHandler::getReq(int fd)
{
	std::map<int, request*>::iterator it = _requests.find(fd);

	if (it == _requests.end())
	{
		delReq(fd);
		_requests[fd] = createMethod(fd);
	}

	return (_requests[fd]);
}

/*
		std::map<int, request*>::iterator it = _requests.find(fd);
		if (it == _requests.end())
		{
			addReq(fd);
			it = _requests.find(fd);
		}
		if (!it->second->readSocket())
			return ;
		it->second->process();
*/

void	requestHandler::readReq(int fd)
{
	request* req;

	try
	{
		req = getReq(fd);
		if (req->readSocket() != FINISHED) return; // Goes out only if the request hasnt been fully readed or an error ocurred
		req->process();
		throw httpResponse(req);
	} // This is where the magic of each method happends

	catch(const httpResponse& e)
	{
		e.sendResponse(fd);
	} // Sends the html for Get Post Delete and Error

	catch(const std::exception &e)
	{
		std::cout << "Error: " << e.what() << std::endl;
	} // Read errors

	delReq(fd); // Once finished clean it
}

/*
void	requestHandler::readReq(int fd)
{
	request* req = getReq(fd);

	try { if (req->readSocket() == NOT_FINISHED) return; } //Goes out if the request hasnt been fully readed

	catch (const std::runtime_error& e) { return (std::cout << "Error: " << e.what() << std::endl, delReq(fd)); }

	try { req->process(); }

	catch(const httpResponse& e) { e.sendResponse(fd); } //sends the html for Get Post Delete and Error

	delReq(fd); // Once finished clean it
}
*/
