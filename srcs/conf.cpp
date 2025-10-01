#include "WebServer.hpp"

std::vector<serverConfig>	conf::_servers;

int			conf::_epfd;
epoll_event	conf::_event;

//---------------------------------------------------------------------------//

std::vector<serverConfig>::iterator	conf::serverBegin()			{ return _servers.begin();	}
std::vector<serverConfig>::iterator	conf::serverEnd()			{ return _servers.end();	}

const int&			conf::epfd()		{ return _epfd;		}
const epoll_event&	conf::event()		{ return _event;	}

void setServerName(serverConfig& config, const std::string& value)
{
	config._serverName = value;
}

void setRoot(serverConfig& config, const std::string& value)
{
	config._root = value;
}

void setBodySize(serverConfig& config, const std::string& value)
{
	config._bodySize = static_cast<size_t>(std::atoi(value.c_str()));
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

void setIndex(serverConfig& config, const std::string& value) { config._index = value; }

struct DirectiveHandler {
	const char* name;
	void (*handler)(serverConfig&, const std::string&);
};

#define HANDLER_SIZE 5

void handleDirective(serverConfig& config, const std::string& key, const std::string& value)
{
	static DirectiveHandler g_directives[] =
	{
		{"listen",					setListen},
		{"server_name",				setServerName},
		{"root",					setRoot},
		{"client_max_body_size",	setBodySize},
		{"index",					setIndex}
	};

	for (int i = 0; i < HANDLER_SIZE; i ++)
		if (g_directives[i].name == key)
			return (g_directives[i].handler(config, value));

	throw std::runtime_error("Unknown directive: '" + key + "' | setConf.cpp - handleDirective()");
}

void setErrorPage(serverConfig& config, const std::string& key, const std::string& value)
{
	if (std::isdigit(key[0]))
	{
		int code = atoi(key.c_str());
		config._errorPages[code] = value;
		return ;
	}
	throw std::runtime_error("Not an error code: " + key + "' | setConf.cpp - setErrorPage()");
}


size_t parseServer(std::vector<std::string> tokens, size_t i, serverConfig& srv)
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
			setErrorPage(srv, key, value);
		else
			handleDirective(srv, key, value);

		if (tokens[i] != ";")
			throw std::runtime_error("';' expected after '" + value + "' | setConf.cpp - parseServer()");
		i ++;
	}

	if (i >= tokens.size() || tokens[i] != "}")
		throw std::runtime_error("Missing closing '}' for server block | setConf.cpp - parseServer()");

	return i;	// return position after '}'
}

static std::vector<std::string> tokenize(const std::string &text)
{
	std::vector<std::string> tokens;
	size_t n = text.size();
	for (size_t i = 0; i < n; )
	{
		char c = text[i];
		// comments
		if (c == '#')
		{
			// skip to newline
			while (i < n && text[i] != '\n') ++i;
			continue;
		}
		if (c == '/' && i + 1 < n && text[i + 1] == '/')
		{
			while (i < n && text[i] != '\n') ++i;
			continue;
		}
		if (isspace((unsigned char)c)) { ++i; continue; }
		if (c == '{' || c == '}' || c == ';')
		{
			std::string t(1,c);
			tokens.push_back(t);
			++i;
			continue;
		}
		// word token
		size_t j = i;
		while (j < n && !isspace((unsigned char)text[j]) && text[j] != '{' && text[j] != '}' && text[j] != ';')
			++j;
		tokens.push_back(text.substr(i, j - i));
		i = j;
	}
	return tokens;
}

void	conf::parseFile(std::string filename)
{
	std::ifstream file(filename.c_str());
	if (!file)
		throw std::runtime_error("Config file not found | setConf.cpp - parseFile()");

	std::ostringstream oss;
	oss << file.rdbuf();	// Reads the whole file
	std::string content = oss.str();

	std::vector<std::string> tokens = tokenize(content);

	_epfd = epoll_create1(0);
	if (_epfd == -1)
		throw std::runtime_error("epoll_create | setConf.cpp - parseFile()");

	for (size_t i = 0; i < tokens.size(); ++i)
	{
		if (tokens[i] == "server")
		{
			if (i + 1 < tokens.size() && tokens[i + 1] == "{")
			{
				serverConfig srv; 
				size_t nextPos = parseServer(tokens, i + 2, srv);
				_servers.push_back(srv);
				i = nextPos; // move past the server block
			}
			else
				throw std::runtime_error("Expected '{' after 'server' | setConf.cpp - parseFile()");
		}
	}
}
