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

static bool timeoutCheck(struct timeval *last_activity, long timeout_ms)
{
	struct timeval now;
	gettimeofday(&now, NULL);

	long seconds = now.tv_sec  - last_activity->tv_sec;
	long usec	 = now.tv_usec - last_activity->tv_usec;

	long elapsed_ms = seconds * 1000 + usec / 1000;

	return (elapsed_ms > timeout_ms);
}

StatusCode	request::endNode()	{ return END; }

StatusCode request::execNode(Nodes& currentNode, const nodeHandler nodes[])
{
	StatusCode code;

	while (1)
	{
		code = (this->*(nodes[currentNode].handler))();

		if (code == END)
			return FINISHED;

		if (code != FINISHED)
			return code;
		
		currentNode = static_cast<Nodes>(static_cast<int>(currentNode) + 1);
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

		_location = _server->getLocation(_path);

 		if(!_location.methodAllowed(_method))
			return METHOD_NOT_ALLOWED;

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
/* 			if (_contentLength > _location.getBodySize())
				return PAYLOAD_TOO_LARGE; */

			
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
	if (_headers.count("Transfer-Encoding")
		&& _headers["Transfer-Encoding"].find("chunked") != std::string::npos )
		return chunkedBody();
	else if (_buffer.length() >= _contentLength)
	{
		if (_contentLength > _location.getBodySize())
			return PAYLOAD_TOO_LARGE;
		_body = _buffer;
		return FINISHED;
	}
	return REPEAT;
}

StatusCode request::chunkedBody()
{
	size_t pos = _buffer.find("\r\n");
	if (pos == std::string::npos)
		return REPEAT;

	std::string sizeStr = _buffer.substr(0, pos);

	// Handle chunk extensions (ignore after ;)
	size_t semi = sizeStr.find(';');
	if (semi != std::string::npos)
		sizeStr = sizeStr.substr(0, semi);

	char *endptr = NULL;
	unsigned long chunkSize = std::strtoul(sizeStr.c_str(), &endptr, 16);
	if (endptr == sizeStr.c_str()) // invalid number
		return BAD_REQUEST;

	if (chunkSize == 0)
	{
		// need "0\r\n\r\n"
		if (_buffer.size() < pos + 4)
			return REPEAT;

		_buffer.erase(0, pos + 4);
		return FINISHED;
	}
	size_t dataStart = pos + 2;
	size_t totalNeeded = dataStart + chunkSize + 2;

	if (_buffer.size() < totalNeeded)
		return REPEAT;

	// Validate CRLF after chunk data
	if (_buffer[dataStart + chunkSize] != '\r' ||
		_buffer[dataStart + chunkSize + 1] != '\n')
		return BAD_REQUEST;

	// Append body
	_body.append(_buffer, dataStart, chunkSize);

	// Enforce body size limit HERE
	if (_body.size() > _location.getBodySize())
		return PAYLOAD_TOO_LARGE;

	_buffer.erase(0, totalNeeded);
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

	return execNode(_currentRead, nodes);
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

std::string autoindexBody(const std::string& _path)
{
	std::string newpath;

	int i = -1;
	while (_path[++i])
		if (!(_path[i] == '/' && _path[i + 1] == '/'))
			newpath += _path[i];

	DIR *dir = opendir(newpath.c_str());
	if (!dir)
		return "";

	std::string html = buildAutoindexHtml(newpath, dir);

	closedir(dir);

	return html;
}

StatusCode	request::fillResponse()
{
	static nodeHandler nodes[] = {
		{ONE,	&request::readResponse	},
		{TWO,	&request::cgi			}
	};

	StatusCode code = (this->*(nodes[_currentResponse].handler))();

	if (code == FINISHED)
	{
		std::stringstream response;
		response << "Content-Length: " << _responseBody.size() << "\r\n\r\n";
		_responseHeader.append(response.str());
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

void request::handleStatusCode(StatusCode code)
{
	if (code > SUCCESS && code < REDIRECTION)
	{
		if (_infile < 0 && _responseBody.empty())
			_responseBody = buildSuccesHtml(code);
		return ;
	}
	if (_infile >= 0)
	{
		close(_infile);
		_infile = -1;
	}
	std::map<int, std::string>::const_iterator it = _location.getErrorPages().find(code);
	if (it != _location.getErrorPages().end())
	{
		_infile = open((_location.getRoot() + it->second).c_str(), O_RDONLY);
		if (_infile >= 0)
			return _responseBody.clear();
	}
	_responseBody = buildErrorHtml(code);
}

void	request::setUpResponse(StatusCode code)
{
	epollMood(_fd, EPOLLOUT);
	_currentFunction = TWO;
	_currentResponse = ONE;

	switch (code)
	{
		case CLIENT_DISCONECTED:	std::cout << "Client Disconnected" << std::endl; return end();
			break;
		case AUTOINDEX:				_responseBody = autoindexBody(_path); code = OK;
			break;
		case CGI:					if (cgiSetup()) code = OK; else code = INTERNAL_SERVER_ERROR;
			break;
		case READ_ERROR:			std::cout << "Read Error" << std::endl; code = INTERNAL_SERVER_ERROR;
			break;
		default:
			break;
	}

	std::stringstream response;
	response
		<< _protocol << " " << code << " " << getReasonPhrase(code).c_str() << "\r\n" 
		<< "Content-Type: " << _contentType << "\r\n"
		<< "Connection: close\r\n";

	_responseHeader = response.str();

	handleStatusCode(code);
}

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
		requestHandler::delCgi(_infile);
		kill(_cgiChild, SIGKILL);
		waitpid(_cgiChild, NULL, 0);
	}
	if (_infile >= 0)
		close(_infile);

	requestHandler::delReq(_fd);
}

void	request::exec()
{
	static nodeHandler	nodes[] = {
		{ONE,	&request::readRequest	},
		{TWO,	&request::fillResponse	},
		{THREE,	&request::sendResponse	},
		{FOUR,	&request::endNode		}
	};

	StatusCode	code = execNode(_currentFunction, nodes);

	if(code == FINISHED && _currentFunction == FOUR)
		return end();

	if (code == REPEAT && _currentFunction != THREE)
	{
		if (timeoutCheck(&_last_activity, TIMEOUT_MS))
			code = GATEWAY_TIMEOUT;
	}
	else
		gettimeofday(&_last_activity, NULL);

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

const std::string& request::getContentType()	const { return _contentType;	}
const std::string& request::getBody()			const { return _body;			}
const std::string& request::getMethod()			const { return _method;			}