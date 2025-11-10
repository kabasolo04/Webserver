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

struct MethodFactory
{
	const char* name;
	request* (*create)(request*);
};

static request* createGet		(request* r) { return new myGet(r); 	}
static request* createPost		(request* r) { return new myPost(r);	}
static request* createDelete	(request* r) { return new myDelete(r);	}

bool requestHandler::transform(int fd, request* baby)
{
	static const MethodFactory factories[] = {
		{"GET",    &createGet},
		{"POST",   &createPost},
		{"DELETE", &createDelete}
	};

	const std::string& method = baby->getMethod();

	for (size_t i = 0; i < sizeof(factories) / sizeof(factories[0]); ++i)
	{
		if (method == factories[i].name)
		{
			request* temp = factories[i].create(baby);
			delReq(fd);
			_requests[fd] = temp;
			return true;
		}
	}
	return false;
}

void	requestHandler::execReq(int fd)
{
	if (_requests.find(fd) != _requests.end())
		_requests[fd]->exec();
}

void	requestHandler::addReq(int fd, serverConfig& server)
{
	if(_requests.find(fd) != _requests.end())
		delReq(fd);

	try { setNonBlocking(fd); } catch(const std::exception& e)
	{
		epoll_ctl(fd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		delReq(fd);
		return (void)(std::cerr << e.what() << '\n');
	}

	epoll_event ev = {};
	ev.data.fd = fd;
	ev.events = EPOLLIN;
	epoll_ctl(conf::epfd(), EPOLL_CTL_ADD, fd, &ev);
	_requests[fd] = new request(fd, server);
}
