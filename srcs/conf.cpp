#include "conf.hpp"

int			conf::_port = 8080;
std::string	conf::_serverName = "myWeb";
std::string	conf::_index = "index.html";
std::string	conf::_root = "./www";

void    conf::setConfig(char* filename)
{

}
const int&  		conf::port()		{ return _port;			}
const std::string&	conf::serverName()	{ return _serverName;	}
const std::string&	conf::index()		{ return _index;		}
const std::string&	conf::root()		{ return _root;			}