#pragma once

#include "WebServer.hpp"

#include <sys/time.h>

#define BUFFER 4096

#define OFF			0
#define ON			1
#define CASCADE_OFF	0
#define CASCADE_ON	1
#define TIMEOUT_OFF	0
#define TIMEOUT_ON	1

enum Nodes
{
	ONE,
	TWO,
	THREE,
	FOUR,
	FIVE
};

enum StatusCode
{
	REPEAT,
	FINISHED,
	END,
	RESPONSE,
	CGI,
	AUTOINDEX,
	OK								= 200,
	CREATED							= 201,
	ERRORS,		
	NO_CONTENT						= 204,
	FOUND							= 302,
	BAD_REQUEST						= 400,
	FORBIDEN						= 403,
	NOT_FOUND						= 404,
	METHOD_NOT_ALLOWED				= 405,
	REQUEST_TIMEOUT					= 408,
	PAYLOAD_TOO_LARGE				= 413,
	UNSUPPORTED_MEDIA_TYPE			= 415,
	REQUEST_HEADER_FIELDS_TOO_LARGE	= 431,
	INTERNAL_SERVER_ERROR			= 500,
	NOT_IMPLEMENTED					= 501,
	GATEWAY_TIMEOUT					= 504,
	LOL								= 999,
	BIG_ERRORS,
	READ_ERROR,
	CLIENT_DISCONECTED
};

//enum Status
//{
//	REPEAT,
//	FINISHED,
//	END,
//	STATUS_CODE
//};

class request;

struct nodeData
{
	Nodes&	currentNode;
	bool	cascade;
	bool	timeout;
};

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

		StatusCode							_code;
	
		Nodes								_currentFunction;
		Nodes								_currentRead;
		Nodes								_currentResponse;

		struct timeval						_last_activity;

		std::string							_responseHeader;
		std::string							_responseBody;
		
		void		printHeaders();

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
		StatusCode	readResponse();
		StatusCode	autoindex();
		StatusCode	cgi();

		StatusCode	fillResponse();
		StatusCode	sendResponse();
//---------------------------------------------------------------------------//
		StatusCode	execNode(nodeData& data, const nodeHandler nodes[]);
//		StatusCode	execNode(Nodes& current, const nodeHandler nodes[], int mode);
//---------------------------------------------------------------------------//

		bool						cgiSetup();
		bool		 				isCgiScript(std::string filename);
		void						execChild(int outPipe[2], int inPipe[2]);
		bool						handleParent(pid_t child, int outPipe[2], int inPipe[2]);
		std::vector<std::string>	build_env(std::string path);

		void		setQuery();
		StatusCode 	handleMultipart();

		StatusCode	endNode();

		void	handleError(StatusCode code);
		void	setUpResponse(StatusCode code);
		
	public:
		request(int fd, serverConfig& server);
		request(const request& other);
		request& operator = (const request& other);
		virtual ~request();

		const std::string&	getContentType()	const;
		const std::string&	getBody()			const;
		const std::string&	getMethod()			const;

		void	exec();
		void	end();
};
