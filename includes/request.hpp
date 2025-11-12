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

enum StatusCode
{
//-------------------- FLAGS
	REPEAT,
	FINISHED,
	END,
	STATUS,
//-------------------- ERRORS
	ERROR,
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
	GATEWAY_TIMEOUT			= 504,
	LOL						= 999,
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
		std::string							_cgiCommand;
		pid_t								_cgiChild;
		
		location							_location;
		size_t								_contentLength;
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

		StatusCode					cgiSetup();
		StatusCode 					handleCgi();
		StatusCode					isCgiScript(std::string filename);
		void						execChild(int outPipe[2], int inPipe[2]);
		bool						handleParent(pid_t child, int outPipe[2], int inPipe[2]);
		std::vector<std::string>	build_env();

		void		nextFunction();
		StatusCode	currentFunction();
		void		response(StatusCode code);
	
		request(const request& other);
		request(int fd, std::string target, location* loc);
		request& operator = (const request& other);
		
	public:
		request(int fd, serverConfig& server);
		virtual ~request();

		const std::string& getContentType() const;
		const std::string& getBody() const;
		const std::string& getMethod() const;

		void	exec();
};
