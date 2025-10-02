#include "WebServer.hpp"

request::request(int fd): _fd(fd), _methodSelected(0) {}

request::request(request& other, std::map <int, serverConfig*>& servers): _methodSelected(1)
{
	*this = other;
	std::map<int, serverConfig*>::iterator serverIt = servers.begin();
	for (; serverIt != servers.end(); ++serverIt)
	{
		serverConfig*& server = serverIt->second;
		std::vector<listenEntry>::iterator listenIt = server->listenBegin();
		for (; listenIt != server->listenEnd(); ++listenIt)
			if (listenIt->_fd == _fd)
				_server = *server;
	}
	setHeaderVars();
}

request::~request() {}

request&	request::operator = (const request& other)
{
	if (this != &other)
	{
		_fd = other._fd;
		_buffer = other._buffer;
		_path = other._path;
		_protocol = other._protocol;
		_headers = other._headers;
		_body = other._body;
		_contentType = other._contentType;
		_server = other._server;
	}
	return (*this);
}

bool	request::readSocket()
{
	char buffer[BUFFER];

	ssize_t len = read(_fd, buffer, sizeof(buffer));

	if (len > 0)
		_buffer.append(buffer, len);

	else if (len == 0)
		throw std::runtime_error("Client Disconnected | request.cpp - readSocket()");

	else if (len < 0)
		return (0);

	return (_buffer.find("\r\n\r\n") != std::string::npos);
}

void	request::process()	{ throw httpResponse(METHOD_NOT_ALLOWED); }

request*	request::selectMethod(std::map <int, serverConfig*>& servers)
{
	size_t pos = _buffer.find(' ');

	if (pos == std::string::npos)
		throw httpResponse(BAD_REQUEST);

	std::string method = _buffer.substr(0, pos);
	if (method == "GET")
		return (new myGet(this, servers));
	if (method == "POST")
		return (new myPost(this, servers));
	if (method == "DELETE")
		return (new myDelete(this, servers));
	return this;
}

bool	request::methodSelected()	{ return _methodSelected; }

/* Gets the path and the protocol version from the request line */
void	request::setReqLineVars()
{
	std::string method, path, protocol;
	std::istringstream iss(_buffer);

	// Read three tokens: METHOD, PATH, PROTOCOL
	if (!(iss >> method >> path >> protocol))
		throw httpResponse(BAD_REQUEST);
	_path = path;
	_protocol = protocol;
}

/* Gets the header variables and puts them into a map */
void		request::setHeaderVars()
{
	setReqLineVars();

	size_t header_start = _buffer.find("\r\n");
	if (header_start == std::string::npos)
		throw httpResponse(BAD_REQUEST);
	header_start += 2;
	size_t header_end;

	while ((header_end = _buffer.find("\r\n", header_start)) != std::string::npos)
	{
		if (header_end == header_start)
			break;

		//if (header_end - header_start > 100000000000000000) //GUARRADA CAMBIARRRRRRRRR!!!!!!!!!!
		//	throw httpResponse(BAD_REQUEST);

		std::string line = _buffer.substr(header_start, header_end - header_start);
		size_t colon = line.find(":");
		if (colon == std::string::npos)
			throw httpResponse(BAD_REQUEST);
		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);
		if (key.empty())
			throw httpResponse(BAD_REQUEST);
		if (!value.empty() && value[0] == ' ')
			value.erase(0, 1); //remove space

		_headers[key] = value;
		header_start = header_end + 2;
	}
	if (_headers.find("Host") == _headers.end())
		throw httpResponse(BAD_REQUEST);
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

const std::string& request::getContentType() const { return _contentType; }
const std::string& request::getBody() const { return _body; }
const std::string& request::getMethod() const { return _method; }
const std::string& request::getPath() const { return _path; }
const std::string& request::getQuery() const { return _query; }

void request::setBody(std::string body) { _body = body; }
void request::setContentType(std::string contentType) { _contentType = contentType; }
