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

int			conf::_epfd;
epoll_event	conf::_event;

//-------------------------------------------------------------------------------------------------------//
// All this default setting will only exists until the config file parsing exists and works as intended. //
//-------------------------------------------------------------------------------------------------------//

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

const int&			conf::epfd()		{ return (_epfd);		}
const epoll_event&	conf::event()		{ return (_event);		}
