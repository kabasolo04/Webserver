#include "WebServer.hpp"

request::request(int fd, serverConfig& server):
	_server(&server),
	_fd(fd),
	_infile(0),
	_body(""),
	_cgiChild(0),
	_location(),
	_contentLength(0),
	_currentFunction(ONE),
	_currentRead(ONE),
	_currentResponse(ONE)
	{
		gettimeofday(&_last_activity, NULL);
	}

request::request(const request& other) { *this = other; }

request::~request() {}

request&	request::operator = (const request& other)
{
	if (this != &other)
	{
		_server				= other._server;
		_fd					= other._fd;
		_infile				= other._infile;
		_method				= other._method;
		_buffer				= other._buffer;
		_path				= other._path;
		_protocol			= other._protocol;
		_headers			= other._headers;
		_body				= other._body;
		_contentType		= other._contentType;
		_location			= other._location;
		_contentLength		= other._contentLength;
		_currentFunction	= other._currentFunction;
		_currentRead		= other._currentRead;
		_cgiChild			= other._cgiChild;
	}
	return (*this);
}

static std::string getReasonPhrase(StatusCode code)
{
	switch (code)
	{
		case OK:						return "OK";
		case NO_CONTENT:				return "No Content";
		case FOUND:						return "Found";
		case BAD_REQUEST:				return "Bad Request";
		case NOT_FOUND:					return "Not Found";
		case INTERNAL_SERVER_ERROR:		return "Internal Server Error";
		case FORBIDEN:					return "Forbiden";
		case METHOD_NOT_ALLOWED:		return "Method Not Allowed";
		case REQUEST_TIMEOUT:			return "Request Timeout";
		case PAYLOAD_TOO_LARGE: 		return "Payload Too Large";
		case UNSUPPORTED_MEDIA_TYPE:	return "Unsupported Media Type";
		case NOT_IMPLEMENTED: 			return "Not Implemented";
		case GATEWAY_TIMEOUT: 			return "Gateway Timeout";
		default:						return "Unknown";
	}
}

static void	epollMood(int fd, uint32_t mood)
{
	struct epoll_event ev;
	ev.events = mood;
	ev.data.fd = fd;
	epoll_ctl(conf::epfd(), EPOLL_CTL_MOD, fd, &ev);
}

StatusCode request::readRequest()
{
	static nodeHandler nodes[] = {
		{ONE,	&request::setUpRequestLine	},
		{TWO,	&request::setUpHeader		},
		{THREE,	&request::setUpBody			},
		{FOUR,	&request::endNode			}
	};

	StatusCode code = myRead(_fd, _buffer);

	if (code >= READ_ERROR)
		return code;

	nodeData data = {_currentRead, CASCADE_ON, TIMEOUT_ON};

	return execNode(data, nodes);
}

struct methodHandler
{
	std::string	method;
	Nodes		node;
};

static Nodes whichMethod(std::string& method)
{
	static methodHandler methods[] = {
		{"GET",		ONE},
		{"POST",	TWO},
		{"DELETE",	THREE}
	};

    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
        if (methods[i].method == method)
            return methods[i].node;

    return FOUR;
}

StatusCode	request::setUpMethod()
{
	static nodeHandler nodes[] = {
		{ONE, 	&request::setUpGet	},
		{TWO,	&request::setUpPost	},
		{THREE,	&request::setUpDel	},
		{FOUR,	&request::endNode	}
	};

	if (isCgiScript(_path) > OK) // See if it's a cgi file and assign the appropiate cgi and execute
		return BAD_REQUEST;

	if (_cgiCommand != "")
		return cgiSetup();

	nodeData data = {whichMethod(_method), CASCADE_OFF, TIMEOUT_OFF};

	return execNode(data, nodes);
}

static StatusCode sendChunk(int fd, const std::string &data)
{
    std::ostringstream chunk;
    chunk << std::hex << data.size() << "\r\n";
    chunk << data << "\r\n";

    std::string out = chunk.str();

    ssize_t ret = send(fd, out.c_str(), out.size(), MSG_NOSIGNAL);

    if (ret == -1 && (errno == EPIPE || errno == ECONNRESET))
        return CLIENT_DISCONECTED;

    return OK;
}

StatusCode	request::response()
{
	static nodeHandler nodes[] = {
		{ONE,	&request::readResponse	},
		{TWO,	&request::autoindex		},
		{THREE,	&request::cgi			},
		{FOUR,	&request::endNode		}
	};

	_buffer.clear();
	
	nodeData data = {_currentResponse, CASCADE_OFF, TIMEOUT_OFF};

	StatusCode code = execNode(data, nodes);

	if (code > OK)
		return code;

	if (sendChunk(_fd, _buffer) != CLIENT_DISCONECTED && code == CLIENT_DISCONECTED)
	{
		write(_fd, "0\r\n\r\n", 5);
		return FINISHED;
	}

	return code;
}

std::string request::handleError(StatusCode code)
{
	if (_infile > 0)
	{
		close(_infile);
		_infile = 0;
	}
	std::stringstream response;
	response << _protocol << " " << code << " " << getReasonPhrase(code).c_str() << "\r\n" << "Content-Type: text/html" << "\r\n";
	
	std::map<int, std::string>::const_iterator it = _location.getErrorPages().find(code);
	if (it != _location.getErrorPages().end())
	{
		_infile = open(it->second.c_str(), O_RDONLY);
		if (_infile > 0)
		{
			_currentResponse = ONE; 
			response << "Transfer-Encoding: chunked\r\n" << "\r\n";
			return response.str().c_str();
		}
	}

	_currentFunction = FOUR;
	response << "Content-Length: " << getReasonPhrase(code).length() + 4 << "\r\n" << "\r\n";
	response << code << " " << getReasonPhrase(code) << "\r\n" << "\r\n";

	return response.str().c_str();
}

std::string request::handleOk()
{
	std::stringstream response;
	response << _protocol << " " << OK << " " << getReasonPhrase(OK).c_str() << "\r\n" << "Content-Type: text/html" << "\r\n";
	
	if (_infile > 0)
	{
		_currentResponse = ONE;
		response << "Transfer-Encoding: chunked\r\n" << "\r\n";
	}
	else
	{
		_currentResponse = FOUR;
		response << "Content-Length: " << _body.size() + 4 << "\r\n" << "\r\n";
		response << _body << "\r\n" << "\r\n";
	}

	return response.str().c_str();
}

void	request::setUpResponse(StatusCode code)
{
	epollMood(_fd, EPOLLOUT);
	_currentFunction = THREE;

	std::string header;

	switch (code)
	{
		case CLIENT_DISCONECTED:	std::cout << "Client Disconnected" << std::endl;	_currentResponse = FOUR;
			break;
		case OK:					header = handleOk();
			break;
		case AUTOINDEX:				_currentResponse = TWO;
			break;
		case CGI:					_currentResponse = THREE;
			break;
		case READ_ERROR:			std::cout << "Read Error" << std::endl;	header = handleError(INTERNAL_SERVER_ERROR);
			break;
		
		default:					header = handleError(code);
			break;
	}

	write(_fd, header.c_str(), header.size());

	std::cout << _fd << "--------------------" << std::endl;
}
//
//void request::setUpResponse()
//{
//
//	epollMood(_fd, EPOLLOUT);
//	_currentFunction = THREE;
//
//	std::stringstream response;
//
//	else
//	{
//		response	<< _protocol << " " << OK << " " << getReasonPhrase(OK).c_str() << "\r\n" << "Content-Type: text/html" << "\r\n";
//
//		if (_code == CGI)
//		{
//			response << "Transfer-Encoding: chunked\r\n" << "\r\n";
//			write(_fd, response.str().c_str(), response.str().size());
//			_currentResponse = THREE;
//			return ;
//		}
//
//		if (_code == AUTOINDEX)
//		{
//			response << "Transfer-Encoding: chunked\r\n" << "\r\n";
//			write(_fd, response.str().c_str(), response.str().size());
//			_currentResponse = TWO;
//			return ;
//		}
//
//		if (_code == OK)
//		{
//			if (_infile > 0)
//			{
//				response << "Transfer-Encoding: chunked\r\n" << "\r\n";
//				write(_fd, response.str().c_str(), response.str().size());
//				_currentResponse = ONE;
//				return ;
//			}
//
//			response << "Content-Length: " << _body.length() << "\r\n" << "\r\n";
//			response << _body;
//
//			write(_fd, response.str().c_str(), response.str().size());
//			_currentFunction = FOUR;
//		}
//		return ;
//	}
//}

void request::exec()
{
	static nodeHandler nodes[] = {
		{ONE, 	&request::readRequest	},
		{TWO,	&request::setUpMethod	},
		{THREE,	&request::response		},
		{FOUR,	&request::endNode		}
	};

	nodeData data = {_currentFunction, CASCADE_ON, TIMEOUT_ON};

	StatusCode code = execNode(data, nodes);

	if (code >= RESPONSE)
		setUpResponse(code);

	if(code == FINISHED)
		end();
}


	//if (_protocol == "HTTP/1.1" || _headers.find("Connection") != _headers.end())
	//{
	//	_method.clear();
	//	_buffer.clear();
	//	_path.clear();
	//	_protocol.clear();
	//	_headers.clear();
	//	_contentType.clear();
	//	_contentLength = 0;
	//	_query.clear();

	//	//_location = ;
	//	_currentFunction = ONE;
	//	_currentRead = ONE;
	//	_code = OK;

	//	epollMood(_fd, EPOLLIN);
	//}
	//else