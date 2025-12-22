#pragma once

#include "WebServer.hpp"

#define BUFFER 10000
#define TIMEOUT_MS 5000	// 5 Seconds

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
	SUCCESS,
	OK								= 200,
	CREATED							= 201,
	ACCEPTED						= 202,
	NON_AUTHORITATIVE_INFORMATION	= 203,
	NO_CONTENT						= 204,
	RESET_CONTENT					= 205,
	PARTIAL_CONTENT					= 206,
	MULTI_STATUS					= 207,
	ALREADY_REPORTED				= 208,
	IM_USED							= 226,
	REDIRECTION,
	MULTIPLE_CHOICES				= 300,
	MOVED_PERMANENTLY				= 301,
	FOUND							= 302,
	SEE_OTHER						= 303,
	NOT_MODIFIED					= 304,
	USE_PROXY						= 305,
	TEMPORARY_REDIRECT				= 307,
	PERMANENT_REDIRECT				= 308,
	CLIENT_ERRORS,
	BAD_REQUEST						= 400,
	UNAUTHORIZED					= 401,
	PAYMENT_REQUIRED				= 402,
	FORBIDDEN						= 403,
	NOT_FOUND						= 404,
	METHOD_NOT_ALLOWED				= 405,
	NOT_ACCEPTABLE					= 406,
	PROXY_AUTHENTICATION_REQUIRED	= 407,
	REQUEST_TIMEOUT					= 408,
	CONFLICT						= 409,
	GONE							= 410,
	LENGTH_REQUIRED					= 411,
	PRECONDITION_FAILED				= 412,
	PAYLOAD_TOO_LARGE				= 413,
	URI_TOO_LONG					= 414,
	UNSUPPORTED_MEDIA_TYPE			= 415,
	RANGE_NOT_SATISFIABLE			= 416,
	EXPECTATION_FAILED				= 417,
	IM_A_TEAPOT						= 418,
	MISDIRECTED_REQUEST				= 421,
	UNPROCESSABLE_ENTITY			= 422,
	LOCKED							= 423,
	FAILED_DEPENDENCY				= 424,
	TOO_EARLY						= 425,
	UPGRADE_REQUIRED				= 426,
	PRECONDITION_REQUIRED			= 428,
	TOO_MANY_REQUESTS				= 429,
	REQUEST_HEADER_FIELDS_TOO_LARGE	= 431,
	UNAVAILABLE_FOR_LEGAL_REASONS	= 451,
	SERVER_ERROR,
	INTERNAL_SERVER_ERROR			= 500,
	NOT_IMPLEMENTED					= 501,
	BAD_GATEWAY						= 502,
	SERVICE_UNAVAILABLE				= 503,
	GATEWAY_TIMEOUT					= 504,
	HTTP_VERSION_NOT_SUPPORTED		= 505,
	VARIANT_ALSO_NEGOTIATES			= 506,
	INSUFFICIENT_STORAGE			= 507,
	LOOP_DETECTED					= 508,
	NOT_EXTENDED					= 510,
	NETWORK_AUTHENTICATION_REQUIRED = 511,
	BIG_ERRORS,
	READ_ERROR,
	CLIENT_DISCONECTED
};

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
	
		Nodes								_currentFunction;
		Nodes								_currentRead;
		Nodes								_currentResponse;

		struct timeval						_last_activity;

		std::string							_responseHeader;
		std::string							_responseBody;
		
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
		StatusCode	cgi();
		StatusCode	fillResponse();
		StatusCode	sendResponse();
//---------------------------------------------------------------------------//
		StatusCode	execNode(Nodes& currentNode, const nodeHandler nodes[]);

		void	printHeaders();
		bool	cgiSetup();
		bool	isCgiScript(std::string filename);
		void	execChild(int outPipe[2], int inPipe[2]);
		bool	handleParent(pid_t child, int outPipe[2], int inPipe[2]);
		void	handleStatusCode(StatusCode code);
		void	setUpResponse(StatusCode code);
		void	setQuery();
		
		StatusCode 	handleMultipart();
		StatusCode	endNode();

		std::vector<std::string>	build_env(std::string path);
		
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

std::string	buildAutoindexHtml(std::string uri, DIR *dir);
std::string	buildErrorHtml(StatusCode code);
std::string buildSuccesHtml(StatusCode code);
std::string	getReasonPhrase(StatusCode code);
