#include "WebServer.hpp"

bool	conf::_autoindex = true;

int		conf::_port = 8080;
int		conf::_server = 0;
int		conf::_bodySize = 200000;
size_t	conf::_headerSize = 8192;

std::string	conf::_root = "./www";
std::string conf::_host = "127.0.0.1";
std::string	conf::_index = "index.html";
std::string	conf::_serverName = "myWeb";

std::vector<std::string> conf::_methods;

//-------------------------------------------------------------------------------------------------------//
// All this default setting will only exists until the config file parsing exists and works as intended. //
//-------------------------------------------------------------------------------------------------------//

int	createServer(int port)
{
	int listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenFd < 0)
		throw std::runtime_error("Socket failed | conf.cpp - createServer()");

	int yes = 1;
	if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
	{
		close(listenFd);
		throw std::runtime_error("Setsockopt failed | conf.cpp - createServer()");
	}

	struct sockaddr_in addr;
	//std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY; // bind to all interfaces
	addr.sin_port = htons(port);

	if (bind(listenFd, (sockaddr*)&addr, sizeof(addr)) < 0)
	{
		close(listenFd);
		throw std::runtime_error("Bind failed | conf.cpp - createServer()");
	}

	if (listen(listenFd, SOMAXCONN) < 0)
	{
		close(listenFd);
		throw std::runtime_error("Listen failed | conf.cpp - createServer()");
	}
	return listenFd;
}

int setNonBlocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;

	if (old_option == -1)
		throw std::runtime_error("fctnl() failed | conf.cpp - setNonBlocking()");
	if (fcntl(fd, F_SETFL, new_option) == -1)
		throw std::runtime_error("fctnl() failed | conf.cpp - setNonBlocking()");
	return 1; 
}

void	conf::setConfig(std::string filename)
{
	_methods.push_back("GET");
	_methods.push_back("POST");
	_methods.push_back("DELETE");

	(void)filename;
	//Configuration file parsing

	_server = createServer(_port);
	setNonBlocking(_server);
}

const bool&			conf::autoindex()	{ return _autoindex;	}

const int&  		conf::port()		{ return _port;			}
const int&  		conf::server()		{ return _server;		}
const int&  		conf::bodySize()	{ return _bodySize;		}
const size_t&  		conf::headerSize()	{ return _headerSize;	}

const std::string&	conf::root()		{ return _root;			}
const std::string&	conf::host()		{ return _host;			}
const std::string&	conf::index()		{ return _index;		}
const std::string&	conf::serverName()	{ return _serverName;	}	

bool	conf::methodAllowed(std::string method)
{
	return (std::find(_methods.begin(), _methods.end(), method) != _methods.end());
}
