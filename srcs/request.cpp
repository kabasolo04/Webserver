#include "WebServer.hpp"

request::request() {}

request::request(int fd, std::string buffer): _fd(fd), _buffer(buffer), _finished(0) {}

request::~request() {}

void	request::readFd()
{
	char buffer[BUFFER];

	ssize_t len = read(_fd, buffer, sizeof(buffer));

	if (len < 0)
		throw std::runtime_error("Read Error");
	if (len == 0)
		throw std::runtime_error("Client Disconnected");
	if (len >= 0)
	{
		_buffer.append(buffer, len);
	}
}

void	request::readSocket()
{
	readFd();
	if (check())
		_finished = 1;
}

bool	request::finished()
{
	return (_finished);
}

/* Gets the path and the protocol version from the request line */
void	request::getReqLineVars()
{
	std::string	buffer = _buffer;
	size_t		i = -1;
	size_t		pos = buffer.find(' ');

	if (pos == std::string::npos)
		throw httpException(BAD_REQUEST);
	while (buffer[++i] != ' ')
		_path += buffer[i];
	pos = buffer.find("\r\n");
	if (pos == std::string::npos)
		throw httpException(BAD_REQUEST);
	std::string _protocol = buffer.substr(i + 1, pos - (i + 1));
}


/* Gets the header variables and puts them into a map */
void		request::getHeaderVars()
{
	getReqLineVars();

	size_t header_start = _buffer.find("\r\n");
	if (header_start == std::string::npos)
		throw httpException(BAD_REQUEST);
	header_start += 2;
	size_t header_end;

	while ((header_end = _buffer.find("\r\n", header_start)) != std::string::npos)
	{
		if (header_end == header_start)
			break;

		if (header_end - header_start > conf::headerSize())
			throw httpException(BAD_REQUEST);

		std::string line = _buffer.substr(header_start, header_end - header_start);
		size_t colon = line.find(":");
		if (colon == std::string::npos)
			throw httpException(BAD_REQUEST);
		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);
		if (key.empty())
			throw httpException(BAD_REQUEST);
		if (!value.empty() && value[0] == ' ')
			value.erase(0, 1); //remove space

		_headers[key] = value;
		header_start = header_end + 2;
	}
	if (_headers.find("Host") == _headers.end())
		throw httpException(BAD_REQUEST);
	//std::cout << "Host=" << _headers["Host"] << std::endl;
}
