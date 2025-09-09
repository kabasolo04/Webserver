#pragma once

#include "WebServer.hpp"

class HttpException : public std::exception
{
	private:
		int			_statusCode;
		std::string _message;
		std::string _fullMessage;

	public:
		HttpException(int code, const std::string& message);
		virtual ~HttpException() throw() {}

		int code() const throw();
		const std::string& message() const throw();
		virtual const char* what() const throw();
};
