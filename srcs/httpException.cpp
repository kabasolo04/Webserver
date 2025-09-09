#include "WebServer.hpp"

HttpException::HttpException(int code, const std::string& message): _statusCode(code), _message(message)
{		
    std::stringstream ss;
	ss << "HTTP " << _statusCode << " - " << _message;
	_fullMessage = ss.str();
}

int HttpException::code() const throw() {
	return _statusCode;
}

const std::string& HttpException::message() const throw() {
	return _message;
}

const char* HttpException::what() const throw() {
	return _fullMessage.c_str();
}
