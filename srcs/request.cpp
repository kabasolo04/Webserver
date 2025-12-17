#include "WebServer.hpp"

request::request(int fd, serverConfig& server):
	_server(&server),
	_fd(fd),
	_infile(-1),
	_body(""),
	_contentType("text/html"),
	_cgiChild(0),
	_location(),
	_contentLength(0),
	_code(OK),
	_currentFunction(ONE),
	_currentRead(ONE),
	_currentResponse(ONE)
	{
		gettimeofday(&_last_activity, NULL);
//		_server->printer();
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

StatusCode	request::endNode()	{ return END; }

//#define TIMEOUT_MS 500000

//static bool timeoutCheck(struct timeval *last_activity, long timeout_ms)
//{
//	struct timeval now;
//	gettimeofday(&now, NULL);
//
//	long seconds  = now.tv_sec  - last_activity->tv_sec;
//	long usec     = now.tv_usec - last_activity->tv_usec;
//
//	long elapsed_ms = seconds * 1000 + usec / 1000;
//
//	return (elapsed_ms > timeout_ms);
//}

StatusCode request::execNode(nodeData& data, const nodeHandler nodes[])
{
	StatusCode code;

	while (1)
	{
		//if (data.timeout == ON && timeoutCheck(&_last_activity, TIMEOUT_MS))
		//	return REQUEST_TIMEOUT;

		code = (this->*(nodes[data.currentNode].handler))();

		if (code == END)
			return FINISHED;

		if (code != FINISHED)
			return code;

		data.currentNode = static_cast<Nodes>(static_cast<int>(data.currentNode) + 1);
		
		//if (data.timeout == ON)
		//	gettimeofday(&_last_activity, NULL);	// Reset the timeout per action finished
		
		if (data.cascade == OFF)
			return code;
	}
}

std::string getReasonPhrase(StatusCode code)
{
	switch (code)
	{
		case OK:								return "OK";
		case CREATED:							return "Created";
		case NO_CONTENT:						return "No Content";
		case FOUND:								return "Found";
		case BAD_REQUEST:						return "Bad Request";
		case NOT_FOUND:							return "Not Found";
		case INTERNAL_SERVER_ERROR:				return "Internal Server Error";
		case FORBIDEN:							return "Forbiden";
		case METHOD_NOT_ALLOWED:				return "Method Not Allowed";
		case REQUEST_TIMEOUT:					return "Request Timeout";
		case PAYLOAD_TOO_LARGE: 				return "Payload Too Large";
		case UNSUPPORTED_MEDIA_TYPE:			return "Unsupported Media Type";
		case REQUEST_HEADER_FIELDS_TOO_LARGE:	return "Request Header Fields Too Large";
		case NOT_IMPLEMENTED: 					return "Not Implemented";
		case GATEWAY_TIMEOUT: 					return "Gateway Timeout";
		case LOL: 								return "No Fucking Idea Mate";
		default:								return "Unknown";
	}
}

static void	epollMood(int fd, uint32_t mood)
{
	struct epoll_event ev;
	ev.events = mood;
	ev.data.fd = fd;
	epoll_ctl(conf::epfd(), EPOLL_CTL_MOD, fd, &ev);
}

static StatusCode	myRead(int fd, std::string& _buffer)
{
	char buffer[BUFFER];

	int len = read(fd, buffer, sizeof(buffer));

	if (len < 0)
		return READ_ERROR;

	if (len == 0)
		return CLIENT_DISCONECTED;

	_buffer.append(buffer, len);
 
	return REPEAT;
}

StatusCode	request::setUpRequestLine()
{
	size_t header_end = _buffer.find("\n");

	if (header_end != std::string::npos)
	{
		std::istringstream iss(_buffer);

		if (!(iss >> _method >> _path >> _protocol))
			return BAD_REQUEST;

		_buffer.erase(0, header_end + 1);

		//_server->printer();
		_location = _server->getLocation(_path);

/* 		if(!_location.methodAllowed(_method))
			return METHOD_NOT_ALLOWED; */

		if (_path.find(_location.getPath()) == 0)	// In case location path is prefix of target
			_path = _location.getRoot() + _path.substr(_location.getPath().size());
		else
			_path = _location.getRoot() + _path;
		return FINISHED;
	}
	return REPEAT;
}

static void trim(std::string& s)
{
	size_t start = 0;
	while (start < s.size() && std::isspace(s[start])) start++;
	size_t end = s.size();
	while (end > start && std::isspace(s[end - 1])) end--;
	s = s.substr(start, end - start);
}

StatusCode request::setUpHeader()
{
	while (true)
	{
		size_t end = _buffer.find("\r\n");
		if (end == std::string::npos)
			return REPEAT;

		std::string line = _buffer.substr(0, end);
		_buffer.erase(0, end + 2);

		if (line.empty())
		{
			if(_headers.find("Content-Length") != _headers.end())
			{
				std::stringstream sstream(_headers["Content-Length"]);
				sstream >> _contentLength;
			}
			if (_contentLength > _location.getBodySize())
				return PAYLOAD_TOO_LARGE;
			return FINISHED;
		}
		size_t colon = line.find(':');
		if (colon == std::string::npos)
			return BAD_REQUEST;

		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);
		trim(key);
		trim(value);

		_headers[key] = value;
	}
}

StatusCode	request::setUpBody()
{
	//printHeaders();
	//std::cout << "-----------------------------------------------------------" << std::endl;
	//std::cout << "Path: " << _path << std::endl;
	//std::cout << "Fd: " << _fd << std::endl;
	//std::cout << "buffer.length() = " << _buffer.length() << std::endl;
	//std::cout << "_contentLength = " << _contentLength << std::endl;
	//std::cout << "_location.getBodySize() = " << _location.getBodySize() << std::endl;

	if (_buffer.length() >= _contentLength)
	{
		_body = _buffer;
		return FINISHED;
	}
	return REPEAT;
}

StatusCode request::readRequest()
{
	static nodeHandler nodes[] = {
		{ONE,	&request::setUpRequestLine	},
		{TWO,	&request::setUpHeader		},
		{THREE,	&request::setUpBody			},
		{FOUR,	&request::setUpMethod		},
		{FIVE,	&request::endNode			}
	};

	StatusCode code = myRead(_fd, _buffer);

	if (code == READ_ERROR)
		return code;

	if (code == CLIENT_DISCONECTED)
		return REPEAT;

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

//	printHeaders();

	setQuery();
	if (isCgiScript(_path))
		return CGI;

	return (this->*(nodes[whichMethod(_method)].handler))();
}

StatusCode request::readResponse()
{
	if (_infile < 0)
		return FINISHED;

	std::string temp;

	StatusCode status = myRead(_infile, temp);

	if (temp.size() == 0)
		return FINISHED;

	if (status == REPEAT)
		_responseBody.append(temp);

	return status;
}

StatusCode request::autoindex()
{
	std::string newpath;

	int i = -1;
	while (_path[++i])
		if (!(_path[i] == '/' && _path[i + 1] == '/'))
			newpath += _path[i];

	DIR *dir = opendir(newpath.c_str());
	if (!dir)
		return INTERNAL_SERVER_ERROR;

	_responseBody = buildAutoindexHtml(newpath, dir);
	closedir(dir);
	return FINISHED;
}

StatusCode	request::fillResponse() //14221312
{
	static nodeHandler nodes[] = {
		{ONE,	&request::readResponse	},
		{TWO,	&request::autoindex		},
		{THREE,	&request::cgi			},
		{FOUR,	&request::endNode		}
	};

	StatusCode code = (this->*(nodes[_currentResponse].handler))();

	if (code == FINISHED)
	{
		std::stringstream response;
		response << "Content-Length: " << _responseBody.size() << "\r\n\r\n";
		_responseHeader.append(response.str());
//		std::cout << _responseBody << std::endl;
		return FINISHED;
	}
	return code;
}

StatusCode request::sendResponse()
{
	if (!_responseHeader.empty())
	{
		ssize_t sent = write(_fd, _responseHeader.c_str(), _responseHeader.size());
		if (sent == -1)
			return FINISHED;
		_responseHeader.erase(0, sent);
		return REPEAT;
	}
	if (!_responseBody.empty())
	{
		ssize_t sent = write(_fd, _responseBody.c_str(), _responseBody.size());
		if (sent == -1)
			return FINISHED;
		_responseBody.erase(0, sent);
		return REPEAT;
	}
	return FINISHED;
}

void request::handleError(StatusCode code)
{
	_currentResponse = ONE;
	_code = code;

	_responseBody = "";

	if (_infile >= 0)
	{
		close(_infile);
		_infile = -1;
	}

	std::map<int, std::string>::const_iterator it = _location.getErrorPages().find(code);
	if (it != _location.getErrorPages().end())
	{
		_infile = open(it->second.c_str(), O_RDONLY);
		if (_infile >= 0)
			return ;
	}
	_responseBody = createErrorBody(code);
}

void	request::setUpResponse(StatusCode code)
{
	epollMood(_fd, EPOLLOUT);
	_currentFunction = TWO;

	std::stringstream response;
	response
		<< _protocol << " " << _code << " " << getReasonPhrase(_code).c_str() << "\r\n" 
		<< "Content-Type: " << _contentType << "\r\n"
		<< "Connection: close\r\n";

	_responseHeader = response.str();
		
//	std::cout << _responseHeader << std::endl;

	switch (code)
	{
		case CLIENT_DISCONECTED:	std::cout << "Client Disconnected" << std::endl; end();
			break;
		case OK:					_currentResponse = ONE;
			break;
		case AUTOINDEX:				_currentResponse = TWO;
			break;
		case CGI:					if (cgiSetup()) _currentResponse = THREE; else handleError(INTERNAL_SERVER_ERROR);
			break;
		case READ_ERROR:			std::cout << "Read Error" << std::endl; handleError(INTERNAL_SERVER_ERROR);
			break;
		default: /* Errors */		handleError(code);
			break;
	}
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

void	request::end()
{
	if (_fd > 0)
	{
		epollMood(_fd, EPOLL_CTL_DEL);
		close(_fd);
	}
	// If there is a running CGI child, terminate it before closing the pipe
	if (_cgiChild > 0)
	{
		kill(_cgiChild, SIGKILL);
		waitpid(_cgiChild, NULL, 0);
		_cgiChild = 0;
	}
	if (_infile >= 0)
		close(_infile);

	requestHandler::delReq(_fd);
}

void request::exec()
{
	static nodeHandler nodes[] = {
		{ONE, 	&request::readRequest	},
		{TWO,	&request::fillResponse	},
		{THREE,	&request::sendResponse	},
		{FOUR,	&request::endNode		}
	};

	nodeData data = {_currentFunction, CASCADE_OFF, TIMEOUT_ON};

	StatusCode code = execNode(data, nodes);

	if(code == FINISHED && _currentFunction == FOUR)
		end();

	if (code >= RESPONSE)
		setUpResponse(code);
}

void	request::printHeaders()
{
	std::cout << "===HEADERS===" << std::endl;
	std::cout << "Path: " << _path << std::endl;
	std::cout << "Protocol: " << _protocol << std::endl;
	std::map<std::string, std::string>::iterator it;
	for (it = _headers.begin(); it != _headers.end(); ++it)
	std::cout << it->first << ": " << it->second << std::endl;
	std::cout << "=============" << std::endl;
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

const std::string& request::getContentType()	const { return _contentType;	}
const std::string& request::getBody()			const { return _body;			}
const std::string& request::getMethod()			const { return _method;			}