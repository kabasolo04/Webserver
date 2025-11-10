#include "WebServer.hpp"

request::request(int fd, serverConfig& server):
	_server(&server),
	_fd(fd),
	_location(),
	_contentLength(-1),
	_currentFunction(READ_REQUEST_LINE)
	{

	}

request::request(const request& other): _location(other._location)
{
	*this = other;

	_currentFunction = READ_BODY;

	_contentLength = 0;
	std::istringstream iss(_headers["content-length"]); 
	iss >> _contentLength;

	if (_contentLength <= 0)
	{
		nextFunction();
		if (_headers.find("transfer-encoding") == _headers.end() || _headers["transfer-encoding"] != "chunked")
			nextFunction();
	}

	_location = _server->getLocation(_path);

	//std::cout << "PATH: " << _path << std::endl;

	std::string temp = _location.getRoot();
	std::string locPath = _location.getPath();

	if (_path.find(locPath) == 0)	// In case location path is prefix of target
		_path = temp + _path.substr(locPath.size());
	else
		_path = temp + _path;
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
	}
	return (*this);
}

static StatusCode	readUntil(int fd, std::string& _buffer, std::string eof)
{
	char buffer[BUFFER];

	int len = read(fd, buffer, sizeof(buffer));

	if (len < 0)
		return READ_ERROR;

	if (len == 0)
		return CLIENT_DISCONECTED;

	_buffer.append(buffer, len);
	
	if (_buffer.find(eof) != std::string::npos)
		return FINISHED;
 
	return REPEAT;
}

static StatusCode	readUntil(int fd, std::string& _buffer, size_t totaLen)
{
	char buffer[BUFFER];

	int len = read(fd, buffer, sizeof(buffer));

	if (len < 0)
		return READ_ERROR;

	if (len == 0)
		return CLIENT_DISCONECTED;

	_buffer.append(buffer, len);

	if (_buffer.length() >= totaLen)
		return FINISHED;
 
	return REPEAT;
}

StatusCode	request::readRequestLine()
{
	StatusCode code = readUntil(_fd, _buffer, "\n");

	if (_buffer.length() > _location.getRequestLineSize())
		return BAD_REQUEST;

	if (code == FINISHED)
	{
		std::istringstream iss(_buffer);

		if (!(iss >> _method >> _path >> _protocol))
			return BAD_REQUEST;

		size_t header_end = _buffer.find("\n");
		_buffer.erase(0, header_end + 1);
	}
	return code;
}

static void trim(std::string& s)
{
	size_t start = 0;
	while (start < s.size() && std::isspace(s[start])) start++;
	size_t end = s.size();
	while (end > start && std::isspace(s[end - 1])) end--;
	s = s.substr(start, end - start);
}

#define PARSED_VARS_PER_LOOP 20

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
			if (!requestHandler::transform(_fd, this))
				return METHOD_NOT_ALLOWED;

			//std::cout << "HEADER" << std::endl;
			return REPEAT;
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

StatusCode	request::readHeader()
{
	StatusCode	code = readUntil(_fd, _buffer, "\r\n\r\n");

	if (_buffer.length() > _location.getHeaderSize())
		return BAD_REQUEST;

	if (code < ERRORS)
		code = setUpHeader();

	return code;
}

StatusCode request::readBody()
{
	StatusCode code = readUntil(_fd, _buffer, _contentLength);

	if (_buffer.length() > _location.getBodySize())
		return BAD_REQUEST;

	return code;
}


StatusCode request::readChunked()
{
	StatusCode	code = readUntil(_fd, _buffer, "\r\n");

	if (code == FINISHED)
	{
		std::cout << "CHUNKED" << std::endl;
		return FINISHED;	// Gotta fill this
	}
	return code;
}

/*
StatusCode request::readChunked()	// Not Finished
{
	StatusCode	code = readUntil(_fd, "\r\n");

	if (code == FINISHED)
	{
		// Parse chunk size (in hex)
		std::string size_str = _buffer.substr(0, line_end);
		std::istringstream iss(size_str);
		size_t chunk_size = 0;
		iss >> std::hex >> chunk_size;
		if (iss.fail())
			throw httpResponse(BAD_REQUEST);

		// Remove size line
		_buffer.erase(0, line_end + 2);

		// Check if we already have the full chunk
		if (_buffer.size() < chunk_size + 2) // +2 for trailing \r\n
			return; // incomplete chunk, wait for next EPOLLIN

		// Append chunk to body
		_body.append(_buffer.substr(0, chunk_size));
		_buffer.erase(0, chunk_size + 2);

		// End of transfer
		if (chunk_size == 0)
		{
			_body = _buffer;
			return FINISHED;
		}

	}
	return REPEAT; // need more data
}
*/

StatusCode	request::setUpMethod()		{ return (FINISHED);	}	// Methods must fill it
StatusCode	request::processMethod()	{ return (BIG_ERRORS);	}	// Methods must fill it

//static void	buildResponse(int fd, StatusCode code)
//{
//	struct epoll_event ev;
//	ev.events = EPOLLOUT | EPOLLET;
//	ev.data.fd = fd;
//	epoll_ctl(conf::epfd(), EPOLL_CTL_MOD, fd, &ev);
//
//	std::stringstream response;
//		response << "HTTP/1.1 " << code << " " << getReasonPhrase(code).c_str() << "\r\n"
//				<< "Content-Type: text/html\r\n"
//				//<< "Content-Length: " << getReasonPhrase(code).size() << "\r\n\r\n"
//				<< getReasonPhrase(code);
//	write(fd, response.str().c_str(), response.str().size());
//}

void	request::response(StatusCode code)
{
	std::cout << "ERRORRRRRRRRR" << std::endl;
	if (code >= BIG_ERRORS)
		return (void)end();
	
	std::map<int, std::string>::const_iterator it = _location.getErrorPages().find(code);

	struct epoll_event ev;
	ev.events = EPOLLOUT;
	ev.data.fd = _fd;
	epoll_ctl(conf::epfd(), EPOLL_CTL_MOD, _fd, &ev);

	std::stringstream response;
			
	response << "HTTP/1.1 " << code << " " << getReasonPhrase(code).c_str() << "\r\n" << "Content-Type: text/html\r\n";

	if (it == _location.getErrorPages().end())
	{
		response << "Content-Length: " << getReasonPhrase(code).length() << "\r\n" << "\r\n";
		_buffer.clear();
		response << code << " " << getReasonPhrase(code);
		write(_fd, response.str().c_str(), response.str().size());
		end();
		return ;
	}
	else
	{
		response << "Transfer-Encoding: chunked\r\n" << "\r\n";
		_buffer.clear();
		_infile = open(it->second.c_str(), O_RDONLY);

		if (_infile < 0)
		{
			perror("open");
			end();
			return ;
		}

		_currentFunction = READ_AND_SEND;
	}
	write(_fd, response.str().c_str(), response.str().size());
}

//static void	readResponse(std::string name)
//{
//
//}

void sendChunk(int fd, const std::string &data)
{
    std::ostringstream chunk;
    chunk << std::hex << data.size() << "\r\n";
    chunk << data << "\r\n";
    write(fd, chunk.str().c_str(), chunk.str().size());
}

StatusCode	request::readAndSend()
{
	char buf[50];
	ssize_t n = read(_infile, buf, sizeof(buf));

	if (n > 0) 
		sendChunk(_fd, std::string(buf, n));

	if (n == 0)
	{
		write(_fd, "0\r\n\r\n", 5);
		return FINISHED;
	}

	return REPEAT;
}

//StatusCode	request::send()
//{
// 	struct epoll_event ev;
// 	ev.events = EPOLLOUT | EPOLLET;
// 	ev.data.fd = _fd;
// 	epoll_ctl(conf::epfd(), EPOLL_CTL_MOD, _fd, &ev);	// Telling epoll we are changing the flow of this socket
// 
// 	write(_fd, _response, _response.size());
//}

StatusCode	request::end()
{
	requestHandler::delReq(_fd);
	epoll_ctl(_fd, EPOLL_CTL_DEL, _fd, NULL);
	close(_fd);
	if (_infile > 0)
		close(_infile);
	return (FINISHED);
}

struct nodeHandler
{
	const Request name;
	StatusCode (request::*handler)();
};

StatusCode	request::currentFunction()
{
	static nodeHandler nodes[END_REQUEST + 1] =
	{
		{READ_REQUEST_LINE, &request::readRequestLine	},
		{READ_HEADER,		&request::readHeader		},
		{READ_BODY,			&request::readBody			},
		{READ_CHUNKED,		&request::readChunked		},
		{METHOD_SET_UP,		&request::setUpMethod		},
		{METHOD_PROCESS,	&request::processMethod		},
		{READ_AND_SEND,		&request::readAndSend		},
		{END_REQUEST,		&request::end				}
	};

	return (this->*nodes[_currentFunction].handler)();
}

static StatusCode getCategory(StatusCode code)
{
	if (code > ERRORS)
		return ERRORS;
	return code;
}

void	request::nextFunction()
{
	_currentFunction = static_cast<Request>(_currentFunction + 1);

	if (_currentFunction == METHOD_SET_UP)
	{ 
		struct epoll_event ev;
		ev.events = EPOLLOUT;
		ev.data.fd = _fd;
		epoll_ctl(conf::epfd(), EPOLL_CTL_MOD, _fd, &ev);	// Telling epoll we are changing the flow of this socket
	}
}

void request::exec()
{
	StatusCode code = currentFunction();

	switch (getCategory(code))
	{
		case	FINISHED:	nextFunction();	break;
		case	ERRORS:		response(code);	break;
		case	END:		end();			break;
		default: /*REPEAT:*/				break;
	}
}








	//if (len <= 0)
	//{
	//	std::cout << "[ERROR]: ";
	//	if (len == 0)
	//		std::cout << "Client Disconnnected" << std::endl;
	//	if (len < 0)
	//		std::cout << "Read Error" << std::endl;
	//	return BIG_ERRORS;
	//}










//void	request::printHeaders()
//{
//	std::cout << "===HEADERS===" << std::endl;
//	std::cout << "Path: " << _path << std::endl;
//	std::cout << "Protocol: " << _protocol << std::endl;
//	std::map<std::string, std::string>::iterator it;
//	for (it = _headers.begin(); it != _headers.end(); ++it)
//	std::cout << it->first << ": " << it->second << std::endl;
//	std::cout << "=============" << std::endl;
//}

const std::string& request::getContentType()	const { return _contentType; }
const std::string& request::getBody()			const { return _body; }
const std::string& request::getMethod()			const { return _method; }


static std::string getReasonPhrase(StatusCode code)
{
	switch (code)
	{
		case OK: return "OK";
		case NO_CONTENT: return "No Content";
		case FOUND: return "Found";
		case BAD_REQUEST: return "Bad Request";
		case NOT_FOUND: return "Not Found";
		case INTERNAL_SERVER_ERROR: return "Internal Server Error";
		case FORBIDEN: return "Forbiden";
		case METHOD_NOT_ALLOWED: return "Method Not Allowed";
		case PAYLOAD_TOO_LARGE: return "Payload Too Large";
		case UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
		case NOT_IMPLEMENTED: return "Not Implemented";
		case GATEWAY_TIMEOUT: return "Gateway Timeout";
		case LOL: return "No Fucking Idea Mate";
		default: return "Unknown";
	}
}