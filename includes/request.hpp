#pragma once

#include "WebServer.hpp"

#define BUFFER 500

enum Request
{
	READ_REQUEST_LINE,
	READ_HEADER,
	READ_BODY,
	READ_CHUNKED,
	METHOD_SET_UP,
	METHOD_PROCESS,
	READ_AND_SEND,
	END_REQUEST
};

/*
TRANSFORM:
	Once the first part of the request is readed, some crucial information is revealed to us such
	us method, http-version, host, etc. Since our class is a vanilla one, created just to read
	the begining of the request, it has to update into the expected one "transforming" itself.

	In order to "transform" it has to create a new request throwing itself as construction
	parameter and then destroying itself, this way a new updated request
*/

enum StatusCode
{
//-------------------- FLAGS
	END,
	REPEAT,
	FINISHED,
	TRANSFORM,
//-------------------- ERRORS
	ERRORS					= 199,
	OK						= 200,
	NO_CONTENT				= 204,
	FOUND					= 302,
	BAD_REQUEST				= 400,
	FORBIDEN				= 403,
	NOT_FOUND				= 404,
	METHOD_NOT_ALLOWED		= 405,
	PAYLOAD_TOO_LARGE		= 413,
	UNSUPPORTED_MEDIA_TYPE	= 415,
	INTERNAL_SERVER_ERROR	= 500,
	NOT_IMPLEMENTED			= 501,
	LOL						= 700,
	BIG_ERRORS,
	READ_ERROR,
	CLIENT_DISCONECTED,
};

class request
{
	private:
		serverConfig*	_server;

	protected:
		int									_fd;
		int									_infile;
		std::string							_method;
		std::string							_buffer;
		std::string							_path;
		std::string							_protocol;
		std::map<std::string, std::string>	_headers;
		std::string							_body;
		std::string							_contentType;
		std::string							_query;
		
		location							_location;
		int									_contentLength;
		Request								_currentFunction;
		
//		StatusCode (request::*_function)();

		//void		setReqLineVars();
		//void		printHeaders();
		
		StatusCode	readRequestLine();
		StatusCode	readHeader();
		StatusCode	setUpHeader();
		StatusCode	readBody();
		StatusCode	readChunked();

		// The methods must to fill this functions, else an error will the shown
		virtual StatusCode	setUpMethod();
		virtual StatusCode	processMethod();

		StatusCode			send();
		StatusCode			readAndSend();
		StatusCode			end();

		void	cgi(std::string command);

		void	nextFunction();
		StatusCode	currentFunction();
		void	response(StatusCode code);
	
		request(const request& other);
		request(int fd, std::string target, location* loc);
		request& operator = (const request& other);
		
	public:
		request(int fd, serverConfig& server);
		virtual ~request();

		const std::string& getContentType()	const;
		const std::string& getBody()		const;
		const std::string& getMethod()		const;
		const std::string& getPath()		const;
		const std::string& getQuery()		const;

		void	setBody(std::string body);
		void	setContentType(std::string contentType);

		void	exec();
};
