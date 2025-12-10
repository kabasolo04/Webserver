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

static std::string getReasonPhrase(StatusCode code)
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

StatusCode	request::endNode()	{ return END; }

#define TIMEOUT_MS 500000

static bool timeoutCheck(struct timeval *last_activity, long timeout_ms)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    long seconds  = now.tv_sec  - last_activity->tv_sec;
    long usec     = now.tv_usec - last_activity->tv_usec;

    long elapsed_ms = seconds * 1000 + usec / 1000;

    return (elapsed_ms > timeout_ms);
}

StatusCode request::execNode(nodeData& data, const nodeHandler nodes[])
{
	StatusCode code;

	while (1)
	{
		if (data.timeout == ON && timeoutCheck(&_last_activity, TIMEOUT_MS))
			return REQUEST_TIMEOUT;

		code = (this->*(nodes[data.currentNode].handler))();

		if (code == END)
			return FINISHED;

		if (code != FINISHED || data.cascade == OFF)
			return code;
		
		if (data.timeout == ON)
			gettimeofday(&_last_activity, NULL);	// Reset the timeout per action finished
	
		data.currentNode = static_cast<Nodes>(static_cast<int>(data.currentNode) + 1);
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
	std::cout << "-----------------------------------------------------------" << std::endl;
	std::cout << "Fd: " << _fd << std::endl;
	std::cout << "buffer.length() = " << _buffer.length() << std::endl;
	std::cout << "_contentLength = " << _contentLength << std::endl;
	std::cout << "_location.getBodySize() = " << _location.getBodySize() << std::endl;

	if (_buffer.length() >= _contentLength)
	{
		_body = _buffer;
		return FINISHED;
	}
/* 	if (_contentLength > _location.getBodySize())
		return PAYLOAD_TOO_LARGE; */
	return REPEAT;
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

	printHeaders();

	setQuery();
	if (isCgiScript(_path))
		return CGI;

	return (this->*(nodes[whichMethod(_method)].handler))();
}

StatusCode request::readResponse()
{
	if (_infile < 0)
		return FINISHED;

    StatusCode status = myRead(_infile, _responseBody);

    if (status == READ_ERROR)
        return status;
	
    if (status == CLIENT_DISCONECTED)
        return FINISHED;

    return REPEAT;
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

    std::ostringstream html;

    html
    << "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
    << "<title>🔥 Index of " << newpath << " 🔥</title>"

    // ====== STYLE ======
    << "<style>"
    "body {"
        "margin:0;"
        "font-family: Arial, sans-serif;"
        "background: #000;"
        "color: #fff;"
        "text-shadow: 0 0 5px #f30;"
        "animation: pulse 3s infinite;"
    "}"
    "h1 {"
        "text-align:center;"
        "padding:20px;"
        "font-size:40px;"
        "color:#ff4500;"
        "text-shadow: 0 0 15px #ff0000, 0 0 5px #ff8c00;"
    "}"
    "pre {"
        "max-width:900px;"
        "margin:auto;"
        "padding:20px;"
        "background:rgba(0,0,0,0.6);"
        "border:solid 1px #f30;"
        "border-radius:10px;"
        "box-shadow:0 0 20px #f30;"
    "}"
    "a {"
        "color:#ffb380;"
        "font-size:20px;"
        "text-decoration:none;"
    "}"
    "a:hover {"
        "color:#fff;"
        "text-shadow: 0 0 10px #ff4500;"
    "}"
    "@keyframes pulse {"
        "0% { background-color: #000; }"
        "50% { background-color: #200; }"
        "100% { background-color: #000; }"
    "}"
    // Mini llamas arriba
    ".firebar {"
        "text-align:center;"
        "font-size:25px;"
        "margin:0;"
        "padding:10px 0;"
        "animation: flame 1s infinite;"
    "}"
    "@keyframes flame {"
        "0% { text-shadow: 0 0 5px #f00; }"
        "50% { text-shadow: 0 0 15px #ff8000; }"
        "100% { text-shadow: 0 0 5px #f00; }"
    "}"
    "</style>"
    << "</head><body>"

    // ====== FIRE BAR ======
    << "<div class=\"firebar\">🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥</div>"

    << "<h1>🔥 Index of " << newpath << " 🔥</h1>"
    << "<pre>";

    if (newpath != "./")
        html << "<a href=\"..\">⟵ Back</a>\n";

    struct dirent *entry;
    while ((entry = readdir(dir)) != (void*)0)
    {
        std::string name = entry->d_name;
        std::string fullPath = newpath + "/" + name;

        if (name[0] == '.')
            continue;

        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
            name += "/";

        html << "<a href=\"" << name << "\">" << name << "</a>\n";
    }

    html << "</pre></body></html>";

    _responseBody = html.str();
    closedir(dir);
    return FINISHED;
}

StatusCode	request::response()
{
	static nodeHandler nodes[] = {
		{ONE,	&request::readResponse	},
		{TWO,	&request::autoindex		},
		{THREE,	&request::cgi			},
		{FOUR,	&request::endNode		}
	};

	StatusCode code = (this->*(nodes[_currentResponse].handler))();

	if (code != REPEAT)
	{
		if (code == FINISHED)
		{
			std::stringstream response;
			response
				<< _protocol << " " << _code << " " << getReasonPhrase(_code).c_str() << "\r\n" 
				<< "Content-Type: " << _contentType << "\r\n"
				<< "Connection: close\r\n"
				<< "Content-Length: " << _responseBody.size() + 4 << "\r\n\r\n"
				<< _responseBody << "\r\n\r\n";

			//std::cout << "FINISIHNG" << std::endl;

			(void)send(_fd, response.str().c_str(), response.str().size(), MSG_NOSIGNAL);
		}
		return FINISHED;
	}
	return code;
}

static std::string createErrorBody(StatusCode code)
{
	std::stringstream temp;
	temp <<
	"<!doctype html>\n"
		"<html lang=\"en\">\n"
		"<head>\n"
		"  <meta charset=\"utf-8\" />\n"
		"  <meta name=\"viewport\" content=\"width=device-width,initial-scale=1\" />\n"
		"  <title>" << code << " " << getReasonPhrase(code) << "</title>\n"
		"  <style>\n"
		"    :root{\n"
		"      --bg1:#0f1724; --bg2:#132033; --card:#0b1220; --accent:#7c5cff; --muted:#9aa6b2;\n"
		"    }\n"
		"    *{box-sizing:border-box}\n"
		"    body{\n"
		"      margin:0; height:100vh; display:flex; align-items:center; justify-content:center;\n"
		"      background:linear-gradient(135deg,var(--bg1),var(--bg2));\n"
		"      color:#e6eef6; font-family:Inter,system-ui,Arial;\n"
		"      padding:32px; -webkit-font-smoothing:antialiased;\n"
		"    }\n"
		"    .card{\n"
		"      width:min(900px,95%);\n"
		"      display:grid; grid-template-columns:1fr 350px; gap:28px;\n"
		"      background:rgba(255,255,255,0.02); border-radius:20px;\n"
		"      padding:32px; box-shadow:0 10px 30px rgba(0,0,0,0.5);\n"
		"      border:1px solid rgba(255,255,255,0.05);\n"
		"    }\n"
		"    .code{ font-size:64px; font-weight:800; margin:0; color:var(--accent); }\n"
		"    .reason{ color:var(--muted); font-size:22px; margin-top:4px; }\n"
		"    .msg{ line-height:1.6; color:#dbe9f8; margin-top:16px; }\n"
		"    .btn{\n"
		"      display:inline-block; margin-top:20px; padding:10px 16px;\n"
		"      border-radius:10px; text-decoration:none; color:#e6eef6;\n"
		"      border:1px solid rgba(255,255,255,0.07);\n"
		"      transition:0.2s ease; font-weight:600;\n"
		"    }\n"
		"    .btn:hover{ transform:translateY(-3px); box-shadow:0 8px 24px rgba(0,0,0,0.4); }\n"
		"    @media(max-width:900px){ .card{ grid-template-columns:1fr; text-align:center; } }\n"
		"  </style>\n"
		"</head>\n"
		"<body>\n"
		"  <main class=\"card\">\n"
		"    <section>\n"
		"      <h1 class=\"code\">" << code << "</h1>\n"
		"      <div class=\"reason\">" << getReasonPhrase(code) << "</div>\n"
		"      <p class=\"msg\">\n"
		"        Something went wrong on the server. The response was <strong>" << code << " " << getReasonPhrase(code) << "</strong>.\n"
		"      </p>\n"
		"      <a class=\"btn\" href=\"/\">Return to homepage</a>\n"
		"    </section>\n"
		"    <aside>\n"
		"      <!-- You can replace this SVG with any graphic you want -->\n"
		"      <svg width=\"100%\" height=\"100%\" viewBox=\"0 0 600 600\">\n"
		"        <defs>\n"
		"          <linearGradient id=\"g\" x1=\"0\" x2=\"1\">\n"
		"            <stop offset=\"0\" stop-color=\"#7c5cff\"/>\n"
		"            <stop offset=\"1\" stop-color=\"#5be7c8\"/>\n"
		"          </linearGradient>\n"
		"        </defs>\n"
		"        <ellipse cx=\"300\" cy=\"520\" rx=\"130\" ry=\"18\" fill=\"rgba(0,0,0,0.45)\"/>\n"
		"        <rect x=\"160\" y=\"160\" width=\"280\" height=\"260\" rx=\"28\" fill=\"#071022\" stroke=\"url(#g)\" stroke-width=\"4\"/>\n"
		"        <circle cx=\"270\" cy=\"250\" r=\"14\" fill=\"#eaf6ff\" />\n"
		"        <circle cx=\"330\" cy=\"250\" r=\"14\" fill=\"#eaf6ff\" />\n"
		"        <rect x=\"270\" y=\"290\" width=\"60\" height=\"8\" rx=\"4\" fill=\"#8fb9ff\" />\n"
		"        <rect x=\"292\" y=\"110\" width=\"16\" height=\"60\" rx=\"8\" fill=\"url(#g)\"/>\n"
		"        <circle cx=\"300\" cy=\"100\" r=\"14\" fill=\"#ffd9f2\" />\n"
		"      </svg>\n"
		"    </aside>\n"
		"  </main>\n"
		"</body>\n"
		"</html>\n";

	return (temp.str());
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
	_currentFunction = THREE;

	switch (code)
	{
		case CLIENT_DISCONECTED:	std::cout << "Client Disconnected" << std::endl; end();
			break;
		case OK:					_currentResponse = ONE;
			break;
		case AUTOINDEX:				_currentResponse = TWO;
			break;
		case CGI:					cgiSetup(); _currentResponse = THREE;
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
		{TWO,	&request::setUpMethod	},
		{THREE,	&request::response		},
		{FOUR,	&request::endNode		}
	};

	nodeData data = {_currentFunction, CASCADE_ON, TIMEOUT_ON};

	StatusCode code = execNode(data, nodes);

	if(code == FINISHED)
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