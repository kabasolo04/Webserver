#pragma once

#include "WebServer.hpp"

#define BUFFER 500

#define CASCADE_OFF	0
#define CASCADE_ON	1

enum Nodes
{
	ONE,
	TWO,
	THREE,
	FOUR
};

//enum ReadRequest
//{
//	READ_REQUEST_LINE,
//	READ_HEADER,
//	READ_BODY,
//	END_READ
//};
//
//enum MethodSetUp
//{
//	GET,
//	POST,
//	DELETE,
//	END_SET_UP
//};
//
//enum HandleResponse
//{
//	READ_AND_SEND,
//	AUTOINDEX,
//	CGI,
//	END_RESPONSE
//};
//
//enum Request
//{
//	READ_REQUEST,
//	METHOD_SET_UP,
//	HANDLE_RESPONSE,
//	END_REQUEST
//};

enum StatusCode
{
//-------------------- FLAGS
	REPEAT,
	FINISHED,
	TRANSFORM,
	END,
	STATUS,
//-------------------- ERRORS
	RESPONSE,
	OK						= 200,
	ERRORS,
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

class request;

struct nodeHandler
{
	const Nodes name;
	StatusCode (request::*handler)();
};


class request
{
	private:
		serverConfig*						_server;
	
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
		size_t								_contentLength;
	
		Nodes								_currentFunction;
		Nodes								_currentRead;
		Nodes								_currentSetUp;
		Nodes								_currentResponse;
		StatusCode							_code;
		
		//void		printHeaders();
		
		StatusCode	execNode(Nodes& current, const nodeHandler nodes[], int mode);

		StatusCode	setUpRequestLine();
		StatusCode	setUpHeader();
		StatusCode	setUpBody();

		StatusCode	setUpGet();
		StatusCode	setUpPost();
		StatusCode	setUpDelete();

		StatusCode	readAndSend();
//		StatusCode	autoindex();
//		StatusCode	cgi();

		StatusCode	readRequest();
		StatusCode	setUpMethod();
		StatusCode	response();

		//void						cgi(std::string command);
		//std::string				isCgiScript(std::string filename);
		//void						execChild(const std::string &command, int outPipe[2], int inPipe[2]);
		//void						handleParent(pid_t child, int outPipe[2], int inPipe[2]);
		//std::vector<std::string>	build_env();

		void		end();
		StatusCode	endNode();
	
		request(const request& other);
		request& operator = (const request& other);
		
	public:
		request(int fd, serverConfig& server);
		virtual ~request();

		const std::string& getContentType() const;
		const std::string& getBody() const;
		const std::string& getMethod() const;

		void	exec();
};
