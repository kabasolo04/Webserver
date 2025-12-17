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
	event.data.fd = entry._fd;

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
				//entry._fd = listenIt->_fd;
				throw std::runtime_error("Port " + entry._host + " already binded by another server | serverConfig.cpp - listenExists()");
				//return (true);
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

static location parseLocation(TOKEN_IT it, TOKEN_IT end, location& _default)
{
	if (it == end)
		throw std::runtime_error("Missing location path");

	std::string path = *it;

	if (!path.empty() && path != "/" && path[path.size() - 1] == '/')
		path = path.substr(0, path.size() - 1);

	location temp(_default);
	temp.setPath(path);

	if (++it == end || *it != "{")
		throw std::runtime_error("Expected '{' after location " + path);

	while (++it != end && *it != "}")
	{
		std::string key = *it;

		if (++it == end)
			throw std::runtime_error("Missing value for directive '" + key + "' in location " + path);
	
		temp.handleDirective(key, it, end);

		if (it == end || *it != ";")
			throw std::runtime_error("';' expected after '" + key + "' in location " + path);
	}
	return temp;
}

static void skipBlock(TOKEN_IT& it, TOKEN_IT end)
{
	if (it == end || *it == "{")
		throw std::runtime_error("Expected location name");

	if (++it == end || *it != "{")
		throw std::runtime_error("Expected '}'");

	int depth = 1;
	++it;

	while (it != end && depth > 0)
	{
		if (*it == "{") depth++;
		else if (*it == "}") depth--;
		++it;
	}

	if (depth != 0)
		throw std::runtime_error("Unclosed block");
}

serverConfig& serverConfig::parseServer(TOKEN_IT& it, TOKEN_IT& end)
{
	std::vector<std::pair<TOKEN_IT, TOKEN_IT > > locationITs;

	while (it != end && *it != "}")
	{
		std::string key = *it++;

		if (it == end)
			throw std::runtime_error("Unexpected end of server block");

		if (key == "location")
		{
			TOKEN_IT loc_begin = it;
			skipBlock(it, end);
			TOKEN_IT loc_end = it;
			locationITs.push_back(std::make_pair(loc_begin, loc_end));
		}
		else
		{
			if (key == "listen") setListen(it);
			else if (key == "server_name") setServerName(it);
			else _default.handleDirective(key, it, end);

			if (it == end || *it != ";")
				throw std::runtime_error("';' expected after directive '" + key + "'");
			++it;
		}
	}
	if (it == end || *it != "}")
		throw std::runtime_error("Missing closing '}' for server block");

	for (size_t i = 0; i < locationITs.size(); ++i)
	{
		location temp = parseLocation(locationITs[i].first, locationITs[i].second, _default);

		if (_locations.find(temp.getPath()) != _locations.end())
			throw std::runtime_error("More than one location with name '" + temp.getPath() + "'");

//		std::cout << "location: " << temp.getPath() << std::endl;

		_locations[temp.getPath()] = temp;
	}

	if (_locations.find("/") == _locations.end())
		_locations["/"] = _default;

	return *this;
}

location& serverConfig::getDefaultLocation() { return (_default); }

location &serverConfig::getLocation(std::string &path)
{
	location *best = NULL;
	size_t bestLen = 0;

	std::map<std::string, location>::iterator it;
	for (it = _locations.begin(); it != _locations.end(); ++it)
	{
		const std::string &locPath = it->first;

		if (path.compare(0, locPath.size(), locPath) == 0)
		{
			if (path.size() == locPath.size() ||
				locPath[locPath.size() - 1] == '/' ||
				path[locPath.size()] == '/')
			{
				if (locPath.size() > bestLen)
				{
					bestLen = locPath.size();
					best = &it->second;
				}
			}
		}
	}
	if (best != NULL)
		return *best;
	return _default;
}

void serverConfig::printer() const
{
	std::cout << "=== SERVER CONFIG ===" << std::endl;

	std::cout << "Server Name: " << _serverName << std::endl;

	// Listen entries
	std::cout << "Listen entries:" << std::endl;
	for (size_t i = 0; i < _listen.size(); ++i)
	{
		std::cout << "  - " << _listen[i]._host << ":" << _listen[i]._port << std::endl;
	}

	// Default location
	std::cout << "\n--- DEFAULT LOCATION ---" << std::endl;
	_default.print();

	// All other locations
	std::cout << "\n--- LOCATIONS (" << _locations.size() << ") ---" << std::endl;

//	for (size_t i = 0; i < _locations.size(); ++i)
//	{
//		std::cout << "\n[Location " << i << "]" << std::endl;
//		_locations[i].print();
//	}

	std::cout << "\n=========================" << std::endl;
}