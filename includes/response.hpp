#pragma once
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

struct Response
{
	Response(std::string full_request);
	~Response();
	const std::string	method;
	const std::string	path;
	const std::string	http_version;
	const unsigned int	status;
	const bool		allowed;
	std::string		body;
	void			parse(std::string full_request)
	const std::string	send_response_header(const int socket_fd);
	const std::string	send_response_body(const int socket_fd);
	/* enum StatusCode */
	/* { */
	/* 	OK = 200, */
	/* 	NotFound = 404, */
	/* 	Forbidden = 403, */
	/* 	MethodNotAllowed = 405, */
	/* 	InternalServerError = 500 */
	/* }; */
};

