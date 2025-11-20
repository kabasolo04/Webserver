#pragma once

#include "WebServer.hpp"

#define BUFFER 5

#define CASCADE_OFF	0
#define CASCADE_ON	1

enum Nodes
{
	ONE,
	TWO,
	THREE,
	FOUR
};

enum StatusCode
{
//-------------------- FLAGS
	REPEAT,
	FINISHED,
	END,
	STATUS,
//-------------------- ERRORS
	RESPONSE,
	CGI,
	AUTOINDEX,
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
	CLIENT_DISCONECTED
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
		std::string							_cgiCommand;
		pid_t								_cgiChild;
		bool								_cgiHeaderCheck;
		
		location							_location;
		size_t								_contentLength;
	
		Nodes								_currentFunction;
		Nodes								_currentRead;
		Nodes								_currentSetUp;
		Nodes								_currentResponse;
		StatusCode							_code;
		
		//void		printHeaders();

//---------------------------------------------------------------------------//
		StatusCode	setUpRequestLine();
		StatusCode	setUpHeader();
		StatusCode	setUpBody();

		StatusCode	readRequest();
//---------------------------------------------------------------------------//
		StatusCode	setUpGet();
		StatusCode	setUpPost();
		StatusCode	setUpDel();

		StatusCode	setUpMethod();
//---------------------------------------------------------------------------//
		StatusCode	readAndSend();
		StatusCode	autoindex();
		StatusCode	cgi();

		StatusCode	response();
//---------------------------------------------------------------------------//
		StatusCode	execNode(Nodes& current, const nodeHandler nodes[], int mode);
//---------------------------------------------------------------------------//


		StatusCode					cgiSetup();
		StatusCode					isCgiScript(std::string filename);
		void						execChild(int outPipe[2], int inPipe[2]);
		bool						handleParent(pid_t child, int outPipe[2], int inPipe[2]);
		std::vector<std::string>	build_env();

		void						setQuery();
		StatusCode 					handleMultipart();

		void 		setUpResponse();
		void		end();
		StatusCode	endNode();
		
	public:
		request(int fd, serverConfig& server);
		request(const request& other);
		request& operator = (const request& other);
		virtual ~request();

		const std::string& getContentType() const;
		const std::string& getBody() const;
		const std::string& getMethod() const;

		void	exec();
};
