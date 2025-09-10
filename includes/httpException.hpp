#pragma once

#include "WebServer.hpp"

class httpException : public std::exception
{
	private:
		int			_statusCode;
		std::string _message;
		std::string _fullMessage;

	public:
		httpException(int code, const std::string& message);
		virtual ~httpException() throw() {}

		int code() const throw();
		const std::string& message() const throw();
		virtual const char* what() const throw();
};

/* #define	BAD_REQUEST			400, "400 - Bad Request"
#define	NOT_FOUND			404, "404 - Not Found"
#define	FORBIDEN			403, "403 - Forbiden"
#define METHOD_NOT_ALLOWED	405, "405 - Method Not Allowed"
#define	INTERNAL_ERROR		500, "500 - Internal Error"
#define LOL					777, "777 - No Fucking Idea Mate"
 */