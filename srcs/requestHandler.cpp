#include "WebServer.hpp"

std::map<int, request*> requestHandler::_requests;

void	requestHandler::delReq(int fd)
{
	std::map<int, request*>::iterator it = _requests.find(fd);
	if (it != _requests.end())
	{
		delete it->second;
		_requests.erase(it);
	}
}

bool requestHandler::transform(int fd, request* baby)
{
	if (baby->getMethod() == "GET")
	{
		request* temp = new myGet(baby);
		delReq(fd);
		_requests[fd] = temp;
		return true;
	}
	if (baby->getMethod() == "POST")
	{
		request* temp = new myPost(baby);
		delReq(fd);
		_requests[fd] = temp;
		return true;
	}
	if (baby->getMethod() == "DELETE")
	{
		request* temp = new myDelete(baby);
		delReq(fd);
		_requests[fd] = temp;
		return true;
	}
	return false;
}

request*	requestHandler::getReq(int fd)
{
	if (_requests.find(fd) == _requests.end())
	{
		throw httpResponse(INTERNAL_SERVER_ERROR);
	}
	return (_requests[fd]);
}

void	requestHandler::addReq(int fd, serverConfig& server)
{
	if(_requests.find(fd) != _requests.end())
		delReq(fd);

	try { setNonBlocking(fd); } catch(const std::exception& e)
	{
		return (void)(std::cerr << e.what() << '\n');
	}

	epoll_event ev = {};
	ev.data.fd = fd;
	ev.events = EPOLLIN;
	epoll_ctl(conf::epfd(), EPOLL_CTL_ADD, fd, &ev);
	_requests[fd] = new request(fd, server);
}

void	requestHandler::readReq(int fd)
{
	try { return getReq(fd)->exec(); }

	catch(const httpResponse& e)
	{
		e.sendResponse(fd);  // Sends the html for all methods and Errors
	}
	catch(const std::exception &e)
	{
		std::cout << e.what() << std::endl; // Catch for strange errors
	}

	delReq(fd);
	epoll_ctl(fd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
}
