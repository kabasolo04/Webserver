#include "WebServer.hpp"

request::request(int fd, std::string target, location& loc): _fd(fd), _target(target), _location(loc)
{
	_function = &request::readHeader;
	_path = _location.getRoot() + target.substr(_location.getPath().size());
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
		_location = other._location;
		_function = other._function;
	}
	return (*this);
}

struct nodeHandler
{
	const nodes name;
	void (request::*handler)();
};

void	request::nextNode(nodes node)
{
	static nodeHandler nodes[STATE_COUNT] =
	{
		{READ_BODY,					&request::readBody},
		{READ_CHUNKED,				&request::readChunked},
		{PROCESS,					&request::process}
	};

	if (node == PROCESS)
	{
		struct epoll_event ev;
		ev.events = EPOLLOUT | EPOLLET;
		ev.data.fd = _fd;
		epoll_ctl(conf::epfd(), EPOLL_CTL_MOD, _fd, &ev);
	}

	_function = nodes[node].handler;
}

/* Gets the header variables and puts them into a map */
void		request::setHeaderVars()
{
	size_t header_start = 0;

	while (1)
	{
		size_t header_end = _buffer.find("\r\n", header_start);
		if (header_end == std::string::npos)
			throw httpResponse(BAD_REQUEST);

		if (header_end == header_start) // empty line -> end of headers
			break;

		if (header_end - header_start > _location.getHeaderSize())
			throw httpResponse(LOL);

		std::string line = _buffer.substr(header_start, header_end - header_start);
		size_t colon = line.find(":");
		if (colon == std::string::npos)
			throw httpResponse(BAD_REQUEST);
		
		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);

		key.erase(0, key.find_first_not_of(" \t"));
		key.erase(key.find_last_not_of(" \t") + 1);
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);

		value.erase(0, value.find_first_not_of(" \t"));
		value.erase(value.find_last_not_of(" \t") + 1);

		if (key.empty())
			throw httpResponse(BAD_REQUEST);

		_headers[key] = value;
		header_start = header_end + 2;
	}
	if (_headers.find("host") == _headers.end())
		throw httpResponse(LOL);

	size_t header_end = _buffer.find("\r\n\r\n");
	if (header_end == std::string::npos)
		header_end = _buffer.find("\n\n");
	_buffer.erase(0, header_end + 4); // remove headers from buffer
}

void	request::readHeader()
{
	char buffer[BUFFER];

	ssize_t len = read(_fd, buffer, sizeof(buffer));

	if (len > 0)
		_buffer.append(buffer, len);
	else if (len == 0)
		throw std::runtime_error("Client Disconnected | request.cpp - readSocket()");

	else if (len < 0)
		return ;

	if (_buffer.find("\r\n\r\n") != std::string::npos)	// HeaderEnd
	{
		setHeaderVars();

		if (_headers.find("content-length") != _headers.end() && _headers["content-length"] != "0")
			nextNode(READ_BODY);
		else if (_headers.find("transfer-encoding") != _headers.end() &&
				_headers["transfer-encoding"] == "chunked")
			nextNode(READ_CHUNKED);
		else
			nextNode(PROCESS);
	}
}

void request::readBody()
{
	char buffer[BUFFER];
	ssize_t len = read(_fd, buffer, sizeof(buffer));

	if (len > 0)
	{
		_buffer.append(buffer, len);
		size_t content_length = 0;
		std::istringstream iss(_headers["content-length"]);
		iss >> content_length;
		if (iss.fail())
			throw httpResponse(BAD_REQUEST);

		if (_buffer.size() >= content_length)
		{
			_body = _buffer;
			nextNode(PROCESS);	// Done reading body
		}
	}
	else if (len == 0)
		throw std::runtime_error("Client disconnected");
	else if (len < 0)
		throw std::runtime_error("Read error");
}

void request::readChunked()
{
	char buffer[BUFFER];
	ssize_t len = read(_fd, buffer, sizeof(buffer));

	if (len > 0)
		_buffer.append(buffer, len);
	else if (len == 0)
		throw std::runtime_error("Client disconnected");
	else if (len < 0 && (errno != EAGAIN && errno != EWOULDBLOCK))
		throw std::runtime_error("Read error");

	// Find the end of the next chunk size line
	size_t line_end = _buffer.find("\r\n");
	if (line_end == std::string::npos)
		return; // need more data

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
		nextNode(PROCESS);
	}
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

void	request::exec() { (this->*_function)(); }