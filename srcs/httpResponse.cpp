#include "WebServer.hpp"

httpResponse::httpResponse(StatusCode code)
{
	_statusCode = code;
	_message = getReasonPhrase(_statusCode);
	_contentType = "text/html";

	std::stringstream temp;
	temp << "<h1>" << code << " " << _message << "</h1>";
	_body = temp.str();
};

httpResponse::httpResponse(const request* req)
{	
	_statusCode = OK;
	_message = getReasonPhrase(_statusCode);
	_contentType = req->getContentType();
	_body = req->getBody();
};

void httpResponse::sendResponse(int fd) const throw()
{
	std::stringstream response;
		response << "HTTP/1.1 " << _statusCode << " " << getReasonPhrase(_statusCode).c_str() << "\r\n"
				<< "Content-Type: " << _contentType << "\r\n"
				<< "Content-Length: " << _body.size() << "\r\n\r\n"
				<< _body;
	write(fd, response.str().c_str(), response.str().size());
}
