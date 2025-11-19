#include "WebServer.hpp"

request::request(int fd, serverConfig& server):
	_server(&server),
	_fd(fd),
	_body(""),
	_location(),
	_contentLength(0),
	_currentFunction(READ_REQUEST),
	_currentRead(READ_REQUEST_LINE),
	_code(OK)
	{}


request::request(const request& other)
{
	*this = other;
}

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
	}
	return (*this);
}

template <typename EnumType>
struct nodeHandler
{
	const EnumType name;
	StatusCode (request::*handler)();
};

template<typename E>
struct EnumTraits;

template<>
struct EnumTraits<Request>		{ static const int count = END_READ; };

template<>
struct EnumTraits<ReadRequest>	{ static const int count = END_REQUEST; };

template<typename E>
static StatusCode nextEnum(E& value)
{
	value = static_cast<E>( static_cast<int>(value) + 1 );
	
	if (value == EnumTraits<E>::count)
		return FINISHED;
	return REPEAT;
}

enum Cascade
{
	CASCADE_OFF,
	CASCADE_ON
};

	template <typename EnumType, std::size_t N>
	StatusCode executeNode(EnumType current, const nodeHandler<EnumType>(&nodes)[N], Cascade mode)
	{
		StatusCode code;

		while (1) // non-blocking cascade
		{
			// Correct syntax for calling a member function pointer
			code = (this->*nodes[current].handler)();

			if (code != FINISHED)
				break;

			if (mode == CASCADE_OFF || nextEnum(current) == FINISHED)
				return FINISHED;

			current = nextEnum(current); // move to next enum state
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
//
//static std::string getReasonPhrase(StatusCode code)
//{
//	switch (code)
//	{
//		case OK:						return "OK";
//		case NO_CONTENT:				return "No Content";
//		case FOUND:						return "Found";
//		case BAD_REQUEST:				return "Bad Request";
//		case NOT_FOUND:					return "Not Found";
//		case INTERNAL_SERVER_ERROR:		return "Internal Server Error";
//		case FORBIDEN:					return "Forbiden";
//		case METHOD_NOT_ALLOWED:		return "Method Not Allowed";
//		case PAYLOAD_TOO_LARGE: 		return "Payload Too Large";
//		case UNSUPPORTED_MEDIA_TYPE:	return "Unsupported Media Type";
//		case NOT_IMPLEMENTED: 			return "Not Implemented";
//		case GATEWAY_TIMEOUT: 			return "Gateway Timeout";
//		case LOL: 						return "No Fucking Idea Mate";
//		default:						return "Unknown";
//	}
//}

StatusCode	request::setUpRequestLine()
{
	if (_buffer.find("\n") != std::string::npos)
	{
		std::istringstream iss(_buffer);

		if (!(iss >> _method >> _path >> _protocol))
			return BAD_REQUEST;

		size_t header_end = _buffer.find("\n");
		_buffer.erase(0, header_end + 1);

		_location = _server->getLocation(_path);

		if (_path.find(_location.getPath()) == 0)	// In case location path is prefix of target
			_path = _location.getRoot() + _path.substr(_location.getPath().size());
		else
			_path = _location.getRoot() + _path;

		std::cout << "Request line finished" << std::endl;
		
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
			std::cout << "Header finished" << std::endl;
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
	if (_buffer.length() >= _contentLength)
	{
		std::cout << "Body finished" << std::endl;
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

	std::cout << "Lets Read" << std::endl;

	if (code > BIG_ERRORS)
		return code;

	static nodeHandler<ReadRequest> nodes[END_READ] = {
		{READ_REQUEST_LINE, &request::setUpRequestLine	},
		{READ_HEADER,		&request::setUpHeader		},
		{READ_BODY,			&request::setUpBody			}
	};

	return executeNode(_currentRead, nodes, CASCADE_ON);
}

StatusCode	request::setUpMethod()	
{
	static nodeHandler<MethodSetUp> nodes[END_SET_UP] = {
		{GET, 		&request::setUpGet	}
		//{POST,		&request::setUpMethod	},
		//{DELETE,	&request::response		}
	};

	return executeNode(GET, nodes, CASCADE_OFF);
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
	
	if (code == CLIENT_DISCONECTED)
	{
		write(_fd, "0\r\n\r\n", 5);
		return END;
	}

	if (code > BIG_ERRORS)
		sendChunk(_fd, _buffer);
	return code;
}

StatusCode	request::response()
{
	static nodeHandler<HandleResponse> nodes[END_RESPONSE] = {
		{READ_AND_SEND, &request::readAndSend	}
		//{AUTOINDEX,		&request::setUpHeader		},
		//{CGI,			&request::setUpBody			}
	};

	return executeNode(READ_AND_SEND, nodes, CASCADE_OFF);
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

void	request::end()
{
	if (_protocol == "HTTP/1.1" || _headers.find("Connection") != _headers.end())
	{
		_method.clear();
		_buffer.clear();
		_path.clear();
		_protocol.clear();
		_headers.clear();
		_contentType.clear();
		_contentLength = 0;
		_query.clear();

		//_location = ;
		_currentFunction = READ_REQUEST;
		_currentRead = READ_REQUEST_LINE;
		_code = OK;
	}
	else
	{
		epollMood(_fd, EPOLL_CTL_DEL);
		//epoll_ctl(_fd, EPOLL_CTL_DEL, _fd, NULL);
		close(_fd);
		requestHandler::delReq(_fd);
	}
	if (_infile > 0)
		close(_infile);
}

void request::exec()
{
	static nodeHandler<Request> nodes[END_REQUEST] = {
		{READ_REQUEST, 		&request::readRequest	},
		{METHOD_SET_UP,		&request::setUpMethod	},
		{HANDLE_RESPONSE,	&request::response		}
	};

	_code = executeNode(_currentFunction, nodes, CASCADE_ON);

	if (_code > RESPONSE)
	{
		_code = response();
		_currentFunction = HANDLE_RESPONSE;
	}
	
	if (_code == END)
		end();
}

const std::string& request::getContentType()	const { return _contentType;	}
const std::string& request::getBody()			const { return _body;			}
const std::string& request::getMethod()			const { return _method;			}

//	if (len == 0) std::cout << "Client Disconnnected" << std::endl;
//	if (len < 0) std::cout << "Read Error" << std::endl;
