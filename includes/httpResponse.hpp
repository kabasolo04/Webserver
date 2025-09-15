#pragma once

enum StatusCode
{
	OK = 200,
	FOUND = 302,
	BAD_REQUEST = 400,
	NOT_FOUND = 404,
	FORBIDEN = 403,
	METHOD_NOT_ALLOWED = 405,
	PAYLOAD_TOO_LARGE = 413,
	UNSUPPORTED_MEDIA_TYPE = 415,
	INTERNAL_SERVER_ERROR = 500,
	LOL = 700

}; //Whenever a code is added you must also add it in utils.cpp - getReasonPhrase()

#include "WebServer.hpp"

class httpResponse: public std::exception
{
	private:
		StatusCode	_statusCode;
		std::string	_message;
		std::string	_contentType;
		std::string _body;

		httpResponse();

	public:
		httpResponse(StatusCode code);
		httpResponse(const request* req);
		virtual ~httpResponse() throw() {}

		void sendResponse(int fd) const throw();
};
