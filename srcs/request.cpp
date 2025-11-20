#include "WebServer.hpp"

request::request(int fd, serverConfig& server):
	_server(&server),
	_fd(fd),
	_body(""),
	_cgiChild(0),
	_location(),
	_contentLength(0),
	_currentFunction(ONE),
	_currentRead(ONE),
	_currentSetUp(ONE),
	_currentResponse(ONE),
	_code(OK)
	{}

request::request(const request& other) { *this = other; }

request::~request() {}

request&	request::operator = (const request& other)
{
	if (this != &other)
	{
		_server				= other._server;
		_fd					= other._fd;
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
		_code				= other._code;
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
		case PAYLOAD_TOO_LARGE: 		return "Payload Too Large";
		case UNSUPPORTED_MEDIA_TYPE:	return "Unsupported Media Type";
		case NOT_IMPLEMENTED: 			return "Not Implemented";
		case GATEWAY_TIMEOUT: 			return "Gateway Timeout";
		case LOL: 						return "No Fucking Idea Mate";
		default:						return "Unknown";
	}
}

StatusCode	request::endNode()	{ return END; }

StatusCode request::execNode(Nodes& current, const nodeHandler nodes[], int mode)
{
	StatusCode code;

	while (1)	// Gotta call the timeout in the future
	{
		code = (this->*(nodes[current].handler))();

		if (code == END)
			return FINISHED;

		if (code != FINISHED || mode == CASCADE_OFF)
			break;

		current = static_cast<Nodes>(static_cast<int>(current) + 1);
	}
	return code;
}

static void	epollMood(int fd, uint32_t mood)
{
	struct epoll_event ev;
	ev.events = mood;
	ev.data.fd = fd;
	epoll_ctl(conf::epfd(), EPOLL_CTL_MOD, fd, &ev);
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
			return FINISHED;

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
	if (_buffer.length() >= _contentLength)
	{
		_body = _buffer;
		return FINISHED;
	}
	if (_buffer.length() >= _location.getBodySize())
		return BAD_REQUEST;
	return REPEAT;
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

StatusCode request::readRequest()
{
	StatusCode code = myRead(_fd, _buffer);

	if (code > BIG_ERRORS)
		return code;

	static nodeHandler nodes[] = {
		{ONE,	&request::setUpRequestLine	},
		{TWO,	&request::setUpHeader		},
		{THREE,	&request::setUpBody			},
		{FOUR,	&request::endNode			}
	};

	return execNode(_currentRead, nodes, CASCADE_ON);
}

StatusCode	request::setUpMethod()
{
	static nodeHandler nodes[] = {
		{ONE, 	&request::setUpGet	},
		{TWO,	&request::setUpPost	},
		{THREE,	&request::setUpDel	},
		{FOUR,	&request::endNode	}
	};

	Nodes node = ONE;

	if (!_location.methodAllowed(_method))
		return METHOD_NOT_ALLOWED;

	if (_method == "GET")
		node = ONE;
	else if (_method == "POST")
		node = TWO;
	else if (_method == "DELETE")
		node = THREE;

	return execNode(node, nodes, CASCADE_OFF);
}

static void sendChunk(int fd, const std::string &data)
{
	std::ostringstream chunk;
	chunk << std::hex << data.size() << "\r\n";
	chunk << data << "\r\n";
	write(fd, chunk.str().c_str(), chunk.str().size());
}

StatusCode	request::readAndSend()
{
	_buffer.clear();
	StatusCode	code = myRead(_infile, _buffer);

	if (code == READ_ERROR)
		return code;

	sendChunk(_fd, _buffer);

	if (code == CLIENT_DISCONECTED)
	{
		write(_fd, "0\r\n\r\n", 5);
		return FINISHED;
	}
	return code;
}

//StatusCode	request::cgi()
//{
//	std::cout << "CGI" << std::endl;
//	return FINISHED;
//}

StatusCode	request::autoindex()
{
	std::cout << "AUTOINDEX" << std::endl;
	return FINISHED;
}

StatusCode	request::response()
{
	static nodeHandler nodes[] = {
		{ONE,	&request::readAndSend	},
		{TWO,	&request::autoindex		},
		{THREE,	&request::cgi			},
		{FOUR,	&request::endNode		}
	};

	return execNode(_currentResponse, nodes, CASCADE_OFF);
}

//StatusCode	request::response()
//{
//	if (_code >= BIG_ERRORS)
//		return std::cout << "BIG ERROR" << std::endl, END;
//	
//	epollMood(_fd, EPOLLOUT);
//
//	std::stringstream response;
//			
//	response	<< _protocol << " " << _code << " " << getReasonPhrase(_code).c_str() << "\r\n"
//				<< "Content-Type: text/html" << "\r\n";
//	
//	if (_protocol == "HTTP/1.1" || (_headers.find("Connection") == _headers.end() && _headers.find("Connection")->second == "keep-alive"))
//		response << "Connection: keep-alive" << "\r\n";
//
//	if (_code == OK)
//	{
//		response << "Content-Length: " << _body.length() << "\r\n" << "\r\n";
//		response << _body;
//	}
//	else
//	{
//		std::map<int, std::string>::const_iterator it = _location.getErrorPages().find(_code);
//		if (it != _location.getErrorPages().end())
//		{
//			_infile = open(it->second.c_str(), O_RDONLY);
//			if (_infile < 0)
//			{
//				perror("open");
//				return END;
//			}
//			response << "Transfer-Encoding: chunked\r\n" << "\r\n";
//			_currentFunction = READ_AND_SEND;
//			write(_fd, response.str().c_str(), response.str().size());
//			return REPEAT;
//		}
//		response << "Content-Length: " << getReasonPhrase(_code).length() + 4 << "\r\n" << "\r\n";
//		response << _code << " " << getReasonPhrase(_code);
//	}
//	write(_fd, response.str().c_str(), response.str().size());
//	return END;
//}


void request::setUpResponse()
{
	if (_code >= BIG_ERRORS)
	{
		_currentFunction = FOUR;
		std::cout << "BIG ERROR" << std::endl;
		return ;
	}

	epollMood(_fd, EPOLLOUT);
	_currentFunction = THREE;

	std::stringstream response;

	if (_code > ERRORS)
	{
		response << _protocol << " " << _code << " " << getReasonPhrase(_code).c_str() << "\r\n" << "Content-Type: text/html" << "\r\n";

		if (_infile > 0)
			close(_infile);

		std::map<int, std::string>::const_iterator it = _location.getErrorPages().find(_code);
		if (it != _location.getErrorPages().end())
		{
			_infile = open(it->second.c_str(), O_RDONLY);
			if (_infile < 0)
			{
				_currentFunction = FOUR;
				return ;
			}
			_currentResponse = ONE;
			response << "Transfer-Encoding: chunked\r\n" << "\r\n";
		}
		else
		{
			response << "Content-Length: " << getReasonPhrase(_code).length() + 4 << "\r\n" << "\r\n";
			response << _code << " " << getReasonPhrase(_code) << "\r\n" << "\r\n";
			write(_fd, response.str().c_str(), response.str().size());
			_currentFunction = FOUR;
			return ;
		}
		write(_fd, response.str().c_str(), response.str().size());
		return ;
	}
	else
	{
		response	<< _protocol << " " << OK << " " << getReasonPhrase(OK).c_str() << "\r\n" << "Content-Type: text/html" << "\r\n";

		if (_code == CGI)
		{
			response << "Transfer-Encoding: chunked\r\n" << "\r\n";
			write(_fd, response.str().c_str(), response.str().size());
			_currentResponse = THREE;
			return ;
		}

		if (_code == AUTOINDEX)
		{
			response << "Transfer-Encoding: chunked\r\n" << "\r\n";
			write(_fd, response.str().c_str(), response.str().size());
			_currentResponse = TWO;
			return ;
		}

		if (_code == OK)
		{
			if (_infile > 0)
			{
				response << "Transfer-Encoding: chunked\r\n" << "\r\n";
				write(_fd, response.str().c_str(), response.str().size());
				_currentResponse = ONE;
				return ;
			}

			response << "Content-Length: " << _body.length() << "\r\n" << "\r\n";
			response << _body;

			write(_fd, response.str().c_str(), response.str().size());
		}
		return ;
	}
}

/*
	if (_code == CGI)
		_currentResponse = THREE;
	if (_code == AUTOINDEX)
		_currentResponse = TWO;

	if (_code > ERRORS)
	{
		
	}
*/

void	request::end()
{
	if (_fd > 0)
	{
		epollMood(_fd, EPOLL_CTL_DEL);
		close(_fd);
	}
	requestHandler::delReq(_fd);
	close(_fd);
	// If there is a running CGI child, terminate it before closing the pipe
	if (_cgiChild > 0)
	{
		kill(_cgiChild, SIGKILL);
		waitpid(_cgiChild, NULL, 0);
		_cgiChild = 0;
	}
	if (_infile > 0)
		close(_infile);
}

void request::exec()
{
	static nodeHandler nodes[FOUR + 1] = {
		{ONE, 	&request::readRequest	},
		{TWO,	&request::setUpMethod	},
		{THREE,	&request::response		},
		{FOUR,	&request::endNode		}
	};

	_code = execNode(_currentFunction, nodes, CASCADE_ON);

	if (_code > RESPONSE)
		setUpResponse();
	
	if (_code == FINISHED)
		end();
}

const std::string& request::getContentType()	const { return _contentType;	}
const std::string& request::getBody()			const { return _body;			}
const std::string& request::getMethod()			const { return _method;			}

//	if (len == 0) std::cout << "Client Disconnnected" << std::endl;
//	if (len < 0) std::cout << "Read Error" << std::endl;




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