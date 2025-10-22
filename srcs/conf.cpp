#include "WebServer.hpp"

std::vector<serverConfig>	conf::_servers;

int			conf::_epfd;
epoll_event	conf::_event;

//---------------------------------------------------------------------------//

std::vector<serverConfig>::iterator	conf::serverBegin()			{ return _servers.begin();	}
std::vector<serverConfig>::iterator	conf::serverEnd()			{ return _servers.end();	}

const int&			conf::epfd()		{ return _epfd;		}
const epoll_event&	conf::event()		{ return _event;	}

static std::vector<std::string> tokenize(const std::string &text)
{
	std::vector<std::string> tokens;
	size_t n = text.size();
	for (size_t i = 0; i < n; )
	{
		char c = text[i];
		if (c == '#')	// Comments
		{
			while (i < n && text[i] != '\n') ++i;	// Skip to newline
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
		size_t j = i;	// Word token
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

	_epfd = epoll_create1(0);
	if (_epfd == -1)
		throw std::runtime_error("epoll_create | setConf.cpp - parseFile()");
	
	std::ostringstream oss;
	oss << file.rdbuf();	// Reads the whole file
	std::string content = oss.str();

	std::vector<std::string> tokens = tokenize(content);

	TOKEN_IT it = tokens.begin();
	TOKEN_IT end = tokens.end();

	while (it != end)
	{
		if (*it == "server")
		{
			it ++;
			if (it == end || *it != "{")
				throw std::runtime_error("Expected '{' after 'server' | setConf.cpp - parseFile()");

			it ++; // move past '{'
			_servers.push_back(serverConfig().parseServer(it, end));
		}
		it ++;
	}
}
