#include "WebServer.hpp"

serverConfig::serverConfig(): _serverName("") {}

serverConfig::~serverConfig() {}

std::vector<listenEntry>::iterator	serverConfig::listenBegin()	{ return _listen.begin();	}
std::vector<listenEntry>::iterator	serverConfig::listenEnd()	{ return _listen.end();		}

const std::string&	serverConfig::serverName()	{ return _serverName;	}

void setServerName(serverConfig& config, const std::string& value)
{
	config._serverName = value;
}

void	setSocket(listenEntry& entry)
{
	entry._fd = socket(AF_INET, SOCK_STREAM, 0);
	if (entry._fd < 0)
		throw std::runtime_error("Socket failed | setConf.cpp - setSocket()");

	int yes = 1;
	if (setsockopt(entry._fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
		return (close(entry._fd), throw std::runtime_error("Setsockopt failed | setConf.cpp - setSocket()"));

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(entry._port);

	if (entry._host.empty() || entry._host == "0.0.0.0")
		addr.sin_addr.s_addr = INADDR_ANY;

	else if (inet_pton(AF_INET, entry._host.c_str(), &addr.sin_addr) <= 0)
		return (close(entry._fd), throw std::runtime_error("Invalid IP address: " + entry._host + " | setConf.cpp - setSocket()"));

	if (bind(entry._fd, (sockaddr*)&addr, sizeof(addr)) < 0)
		return (close(entry._fd), throw std::runtime_error("Bind failed | setConf.cpp - setSocket()"));

	if (listen(entry._fd, SOMAXCONN) < 0)
		return (close(entry._fd), throw std::runtime_error("Listen failed | setConf.cpp - setSocket()"));

	setNonBlocking(entry._fd);

	struct epoll_event	event;
	event.events = EPOLLIN;		// Only watching for input events
	event.data.fd = entry._fd;	// The fd its gonna watch

	if (epoll_ctl(conf::epfd(), EPOLL_CTL_ADD, entry._fd, &event) == -1)
		throw std::runtime_error("epoll_ctl failed | setConf.cpp - setSocket()");
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
				return (true);
			}
		}
	}
	return false;
}

void setListen(serverConfig& config, const std::string& value)
{
	listenEntry entry;
	std::string	temp(value);
	size_t colonPos = temp.find(':');	// Find if we have an IP:PORT format
	
	if (colonPos != std::string::npos)
	{
		entry._host = value.substr(0, colonPos);	// Part before colon = host, by default in 0.0.0.0
		temp = value.substr(colonPos + 1);			// After colon = port
	}

	entry._port = atoi(temp.c_str());
	if (entry._port <= 0) // atoi returns 0 if invalid
		throw std::runtime_error("Invalid port: " + temp + " | setConf.cpp - setLhisten()");

	if (entry._port <= 0 || entry._port > 65535)
	{
		std::ostringstream oss;
		oss << "Port out of range: " << entry._port << " | setConf.cpp - setListen()";
		throw std::runtime_error(oss.str());
	}

	if (!listenExists(entry))
		setSocket(entry);

	config._listen.push_back(entry);
}

size_t serverConfig::parseServer(std::vector<std::string> tokens, size_t i)
{
	while (i < tokens.size() && tokens[i] != "}")
	{
		std::string key = tokens[i++];

		if (i >= tokens.size())
			throw std::runtime_error("Missing value for directive '" + key + "' | setConf.cpp - parseServer()");

		if (key == "listen") setListen(*this, tokens[i++]);

		else if (key == "server_name") _serverName = tokens[i++];

		else i += _default.handleDirective(key, tokens, i);

		if (tokens[i] != ";")
			throw std::runtime_error("';' expected after '" + tokens[i] + "' | setConf.cpp - parseServer()");
		i ++;
	}

	if (i >= tokens.size() || tokens[i] != "}")
		throw std::runtime_error("Missing closing '}' for server block | setConf.cpp - parseServer()");

	return i;	// return position after '}'
}

/*
size_t serverConfig::parseServer(std::vector<std::string> tokens, size_t i)
{
	while (i < tokens.size() && tokens[i] != "}")
	{
		bool errorPage = (tokens[i] == "error_page");

		i += errorPage;
		std::string key = tokens[i++];
		
		if (i >= tokens.size())
		throw std::runtime_error("Missing value for directive '" + key + "' | setConf.cpp - parseServer()");
	
		std::string value = tokens[i++];
		
		std::cout << "Key: " << key << " ";
		std::cout << "Value: " << value << std::endl;

		if (errorPage)
			_default.setErrorPage(key, value);
		else
			_default.handleDirective(key, value);

		if (tokens[i] != ";")
			throw std::runtime_error("';' expected after '" + value + "' | setConf.cpp - parseServer()");
		i ++;
	}

	if (i >= tokens.size() || tokens[i] != "}")
		throw std::runtime_error("Missing closing '}' for server block | setConf.cpp - parseServer()");

	return i;	// return position after '}'
}
*/
