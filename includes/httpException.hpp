#pragma once

#include "WebServer.hpp"

class httpException : public std::exception
{
	private:
		int			_statusCode;

	public:
		httpException(int code);
		virtual ~httpException() throw() {}

		int code() const throw();
};

#define	OK					200
#define	BAD_REQUEST			400
#define	NOT_FOUND			404
#define	FORBIDEN			403
#define METHOD_NOT_ALLOWED	405
#define	INTERNAL_ERROR		500
#define LOL					777
