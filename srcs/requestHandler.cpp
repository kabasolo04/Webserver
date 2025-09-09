#include "WebServer.hpp"

std::map<int, request*> requestHandler::_requests;

void	requestHandler::delReq(int fd)
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
		throw httpException(BAD_REQUEST);
	std::string raw(buffer);
	size_t pos = raw.find(' ');
	if (pos == std::string::npos)
		throw httpException(BAD_REQUEST);

	std::string method = raw.substr(0, pos);
	std::string rest = raw.substr(pos + 1); // everything after the space
	if (!conf::methodAllowed(method))
		throw httpException(METHOD_NOT_ALLOWED);
	if (method == "GET")
		return new myGet(fd, rest);
	if (method == "POST")
		return new myPost(fd, rest);
	if (method == "DELETE")
		return new myDelete(fd, rest);
	throw httpException(METHOD_NOT_ALLOWED);
}

void	requestHandler::addReq(int fd)
{
	delReq(fd);
	_requests[fd] = createMethod(fd);
}

void	requestHandler::readReq(int fd)
{
	try
	{
		std::map<int, request*>::iterator it = _requests.find(fd);
		if (it == _requests.end())
		{
			addReq(fd);
			it = _requests.find(fd);
		}
		it->second->readSocket();
		if (it->second->finished())
		{
			it->second->process();
			delReq(fd);
		}
	}
	catch(const httpException& e)
	{
		//std::cerr << "Error: " << e.what() << std::endl;

		std::stringstream response;
		response << "HTTP/1.1 " << e.code() << " " << e.message().c_str() << "\r\n"
				<< "Content-Type: text/plain\r\n"
				<< "Content-Length: " << e.message().size() << "\r\n\r\n"
				<< e.message();
		write(fd, response.str().c_str(), response.str().size());
		delReq(fd);
	}
//	catch(const otherException& e)
//	{
//		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL); // <-- Step 1
//		close(client_fd);
//		delReq(fd);
//	}
}
