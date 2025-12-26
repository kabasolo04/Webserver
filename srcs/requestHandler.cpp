#include "WebServer.hpp"

std::map<int, request*>	requestHandler::_requests;
std::map<int, bool>		requestHandler::_cgi;

void requestHandler::freeAll()
{
	for (std::map<int, request*>::iterator it = _requests.begin();
		 it != _requests.end(); ++it)
	{
		delete it->second;
	}
	_requests.clear();
}

void	requestHandler::delReq(int fd)
{
	std::map<int, request*>::iterator it = _requests.find(fd);
	if (it != _requests.end())
	{
		delete it->second;
		_requests.erase(it);
	}
}

void	requestHandler::execReq(int fd)
{
	if (_requests.find(fd) != _requests.end())
	{
		if (_requests[fd]->exec() == false)	// Ended
			delReq(fd);
	}
	else if (_cgi.find(fd) != _cgi.end())
		setCgi(fd, true);
}

void	requestHandler::addReq(int fd, serverConfig& server)
{
	if (setNonBlocking(fd) == false)
	{
		epoll_ctl(conf::epfd(), EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		return ;
	}
	epoll_event ev = {};
	ev.data.fd = fd;
	ev.events = EPOLLIN;
	epoll_ctl(conf::epfd(), EPOLL_CTL_ADD, fd, &ev);
	_requests[fd] = new request(fd, server);
}

bool	requestHandler::getCgi(int fd)
{
	if (_cgi.find(fd) != _cgi.end())
		return _cgi[fd];
	return false;
}

void	requestHandler::addCgi(int fd)
{
	_cgi[fd] = false;
	epoll_event ev = {};
	ev.data.fd = fd;
	ev.events = EPOLLIN;
	epoll_ctl(conf::epfd(), EPOLL_CTL_ADD, fd, &ev);
}

void	requestHandler::setCgi(int fd, bool mood)
{
	if (_cgi.find(fd) != _cgi.end())
		_cgi[fd] = mood;
}

void	requestHandler::delCgi(int fd)
{
	_cgi.erase(fd);
	epoll_ctl(conf::epfd(), EPOLL_CTL_DEL, fd, NULL);
}
