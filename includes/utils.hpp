#pragma once

#include "WebServer.hpp"

enum StatusCode
{
	OK = 200,
	BAD_REQUEST = 400,
	NOT_FOUND = 404,
	INTERNAL_SERVER_ERROR = 500,
	FORBIDEN = 403,
	METHOD_NOT_ALLOWED = 405,
	LOL = 700
};


bool		is_directory(const std::string &path);
bool		is_file(const std::string &path);
std::string	getReasonPhrase(StatusCode code);
std::string	buildResponse(StatusCode code, const std::string& body, const std::string& contentType = "text/html");