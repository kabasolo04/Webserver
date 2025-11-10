#include "WebServer.hpp"

static std::string getReasonPhrase(StatusCode code)
{
	switch (code)
	{
		case OK: return "OK";
		case NO_CONTENT: return "No Content";
		case FOUND: return "Found";
		case BAD_REQUEST: return "Bad Request";
		case NOT_FOUND: return "Not Found";
		case INTERNAL_SERVER_ERROR: return "Internal Server Error";
		case FORBIDEN: return "Forbiden";
		case METHOD_NOT_ALLOWED: return "Method Not Allowed";
		case PAYLOAD_TOO_LARGE: return "Payload Too Large";
		case UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
		case NOT_IMPLEMENTED: return "Not Implemented";
		case GATEWAY_TIMEOUT: return "Gateway Timeout";
		case LOL: return "No Fucking Idea Mate";
		default: return "Unknown";
	}
}

std::string buildResponse(StatusCode code, const std::string& body, const std::string& contentType)
{
	std::ostringstream oss;
	oss << "HTTP/1.1 " << code << " " << getReasonPhrase(code) << "\r\n";
	oss << "Content-Type: " << contentType << "\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";
	oss << "Connection: close\r\n";
	oss << "\r\n";
	
	std::string response = oss.str();
	response.append(body);
	return response;
}

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
