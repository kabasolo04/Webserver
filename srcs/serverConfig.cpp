#include "WebServer.hpp"

serverConfig::serverConfig(): _serverName("") {}

serverConfig::serverConfig(const serverConfig& other)	{ *this = other; }

serverConfig& serverConfig::operator=(const serverConfig& other)
{
	if (this != &other)
	{
		_listen = other._listen;
		_serverName = other._serverName;
		_default = other._default;
		_locations = other._locations;
	}
	return *this;
}

serverConfig::~serverConfig() {}

std::vector<listenEntry>::iterator	serverConfig::listenBegin()	{ return _listen.begin();	}
std::vector<listenEntry>::iterator	serverConfig::listenEnd()	{ return _listen.end();		}

const std::string&	serverConfig::serverName()	{ return _serverName;	}

bool setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return false;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

void	setSocket(listenEntry& entry)
{
	entry._fd = socket(AF_INET, SOCK_STREAM, 0);
	if (entry._fd < 0)
		throw std::runtime_error("Socket failed | serverConfig.cpp - setSocket()");

	int yes = 1;
	if (setsockopt(entry._fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
		return (close(entry._fd), throw std::runtime_error("Setsockopt failed | serverConfig.cpp - setSocket()"));

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(entry._port);

	if (entry._host.empty() || entry._host == "0.0.0.0")
		addr.sin_addr.s_addr = INADDR_ANY;

	else if (inet_pton(AF_INET, entry._host.c_str(), &addr.sin_addr) <= 0)
		return (close(entry._fd), throw std::runtime_error("Invalid IP address: " + entry._host + " | serverConfig.cpp - setSocket()"));

	if (bind(entry._fd, (sockaddr*)&addr, sizeof(addr)) < 0)
		return (close(entry._fd), throw std::runtime_error("Bind failed | serverConfig.cpp - setSocket()"));

	if (listen(entry._fd, SOMAXCONN) < 0)
		return (close(entry._fd), throw std::runtime_error("Listen failed | serverConfig.cpp - setSocket()"));

	if (setNonBlocking(entry._fd) == false)
		return (close(entry._fd), throw std::runtime_error("setNonBlocking() failed | serverConfig.cpp - setSocket()"));

	struct epoll_event	event;
	event.events = EPOLLIN;		// Only watching for input events
	event.data.fd = entry._fd;	// The fd its gonna watch

	if (epoll_ctl(conf::epfd(), EPOLL_CTL_ADD, entry._fd, &event) == -1)
		throw std::runtime_error("epoll_ctl failed | serverConfig.cpp - setSocket()");
}

bool listenExists(listenEntry& entry)
{
	std::vector<serverConfig>::iterator serverIt = conf::serverBegin();
	for (; serverIt != conf::serverEnd(); ++serverIt)
	{
		serverConfig& server = *serverIt;

		std::vector<listenEntry>::iterator listenIt = server.listenBegin();
		for (; listenIt != server.listenEnd(); ++listenIt)
		{
			if (listenIt->_host == entry._host && listenIt->_port == entry._port)
			{
				entry._fd = listenIt->_fd;
				throw std::runtime_error("more than one server trying to listen to the same port | serverConfig.cpp - listenExists()");
				return (true);
			}
		}
	}
	return false;
}

void serverConfig::setListen(TOKEN_IT& it)
{
	listenEntry entry;
	const std::string& value = *it;
	std::string	temp(value);
	size_t colonPos = temp.find(':');	// Find if we have an IP:PORT format
	
	if (colonPos != std::string::npos)
	{
		entry._host = value.substr(0, colonPos);	// Part before colon = host, by default in 0.0.0.0
		temp = value.substr(colonPos + 1);			// After colon = port
	}

	entry._port = atoi(temp.c_str());
	if (entry._port <= 0) // atoi returns 0 if invalid
		throw std::runtime_error("Invalid port: " + temp + " | serverConfig.cpp - setListen()");

	if (entry._port <= 0 || entry._port > 65535)
	{
		std::ostringstream oss;
		oss << "Port out of range: " << entry._port << " | serverConfig.cpp - setListen()";
		throw std::runtime_error(oss.str());
	}

	if (!listenExists(entry))
		setSocket(entry);

	_listen.push_back(entry);
	it ++;
}

void	serverConfig::setServerName(TOKEN_IT& it)
{
	_serverName = *it;
	it ++;
}

void	serverConfig::setLocation(TOKEN_IT& it, TOKEN_IT& end)
{
	location	temp(_default, 1);

	if (*it == "{")
		throw std::runtime_error("Argument expected between 'location' and '{' | serverConfig.cpp - setLocation()");

	std::string path = *it;
	temp.setPath(path);

	if (*++it != "{")
		throw std::runtime_error("Argument expected after 'location" + path + "' and '{' | serverConfig.cpp - setLocation()");

	while (++it != end && *it != "}")
	{
		std::string key = *it;

		if (++it == end)
			throw std::runtime_error("Missing value for directive '" + key + "' | serverConfig.cpp - parseServer()");

		temp.handleDirective(key, it, end);

		if (it == end)
			throw std::runtime_error("Unexpected end of file while parsing server block | serverConfig.cpp - parseServer()");
		if (*it != ";")
			throw std::runtime_error("';' expected after '" + key + "' | serverConfig.cpp - setLocation()");
	}

	if (it == end || *it != "}")
		throw std::runtime_error("Missing closing '}' for location block | serverConfig.cpp - setLocation()");

 	it ++;
	
	_locations.push_back(temp);
}

serverConfig& serverConfig::parseServer(TOKEN_IT& it, TOKEN_IT& end)
{
	while (it != end && *it != "}")
	{
		std::string key = *it;

		if (++it == end || *it == "}")
			throw std::runtime_error("Missing value for directive '" + key + "' | serverConfig.cpp - parseServer()");

		if (key == "location") { setLocation(it, end); continue; }	// Ends in }, and not in ; thats the reason for the continue

		else if (key == "listen") setListen(it);

		else if (key == "server_name") setServerName(it);

		else _default.handleDirective(key, it, end);

		if (it == end)
			throw std::runtime_error("Unexpected end of file while parsing server block | serverConfig.cpp - parseServer()");

		if (*it != ";")
			throw std::runtime_error("';' expected after '" + *it  + "' | serverConfig.cpp - parseServer()");
		it ++;	// Jump the ';'
	}

	if (it == end || *it != "}")
		throw std::runtime_error("Missing closing '}' for server block | serverConfig.cpp - parseServer()");

	return *this;
}

location& serverConfig::getDefaultLocation() { return (_default);	}

location& serverConfig::getLocation(std::string path)
{
	location* bestMatch = &_default;
	size_t bestLength = 0;

	for (size_t i = 0; i < _locations.size(); i++)
	{
		const std::string& locPath = _locations[i].getPath();

		if (path.find(locPath) == 0 && locPath.size() > bestLength)
		{
			bestLength = locPath.size();
			bestMatch = &_locations[i];
		}
	}

	return *bestMatch;
}
