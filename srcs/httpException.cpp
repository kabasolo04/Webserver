#include "WebServer.hpp"

httpException::httpException(int code, const std::string& message): _statusCode(code), _message(message)
{		
    std::stringstream ss;
	ss << "HTTP " << _statusCode << " - " << _message;
	_fullMessage = ss.str();
}

int httpException::code() const throw() {
	return _statusCode;
}

const std::string& httpException::message() const throw() {
	return _message;
}

const char* httpException::what() const throw() {
	return _fullMessage.c_str();
}
