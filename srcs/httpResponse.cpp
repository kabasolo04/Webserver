#include "WebServer.hpp"

httpResponse::httpResponse(StatusCode code)
{
	_statusCode = code;
	_contentType = "text/html";

	std::stringstream temp;
	temp << "<h1>" << code << " " << getReasonPhrase(_statusCode) << "</h1>";
	_body = temp.str();
};
/*
httpResponse::httpResponse(StatusCode code, location& loc)
{
	_statusCode = code;
	_contentType = "text/html";

	std::stringstream temp;
	temp << "<h1>" << code << " " << getReasonPhrase(_statusCode) << "</h1>";
	_body = temp.str();
}
*/

httpResponse::httpResponse(request* req)
{	
	_statusCode = OK;
	_contentType = req->getContentType();
	_body = req->getBody();
};

void httpResponse::sendResponse(int fd) const throw()
{
	struct epoll_event ev;
	ev.events = EPOLLOUT | EPOLLET;
	ev.data.fd = fd;
	epoll_ctl(conf::epfd(), EPOLL_CTL_MOD, fd, &ev);

	std::stringstream response;
		response << "HTTP/1.1 " << _statusCode << " " << getReasonPhrase(_statusCode).c_str() << "\r\n"
				<< "Content-Type: " << _contentType << "\r\n"
				<< "Content-Length: " << _body.size() << "\r\n\r\n"
				<< _body;
	write(fd, response.str().c_str(), response.str().size());
}
