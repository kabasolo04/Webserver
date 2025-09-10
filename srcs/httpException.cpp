#include "WebServer.hpp"

httpException::httpException(StatusCode other): _statusCode(other)
{		
};

StatusCode httpException::code() const throw() {
	return _statusCode;
};
