#pragma once

#include "WebServer.hpp"

//enum StatusCode
//{
//	BIG_ERROR = -2,
//	NOT_FINISHED = -1,
//	OK = 200,
//	NO_CONTENT = 204,
//	FOUND = 302,
//	BAD_REQUEST = 400,
//	FORBIDEN = 403,
//	NOT_FOUND = 404,
//	METHOD_NOT_ALLOWED = 405,
//	PAYLOAD_TOO_LARGE = 413,
//	UNSUPPORTED_MEDIA_TYPE = 415,
//	INTERNAL_SERVER_ERROR = 500,
//	NOT_IMPLEMENTED = 501,
//	LOL = 700
//
//}; //Whenever a code is added you must also add it in utils.cpp - getReasonPhrase()

class httpResponse: public std::exception
{
	private:
		StatusCode	_statusCode;
		std::string	_contentType;
		std::string _body;

		httpResponse();

	public:
		httpResponse(StatusCode code);
		//httpResponse(StatusCode code, location& loc);
		httpResponse(request* req);
		virtual ~httpResponse() throw() {}

		void sendResponse(int fd) const throw();
};
