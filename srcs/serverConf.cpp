#include "WebServer.hpp"

serverConfig::serverConfig():
			_serverName(""),
			_root("./www"),
			_bodySize(1048576)
			{}

serverConfig::~serverConfig() {}

std::vector<listenEntry>::iterator	serverConfig::listenBegin()	{ return _listen.begin();	}
std::vector<listenEntry>::iterator	serverConfig::listenEnd()	{ return _listen.end();		}

const std::string&	serverConfig::root()		{ return _root;			}
const size_t&  		serverConfig::bodySize()	{ return _bodySize;		}
const std::string&	serverConfig::serverName()	{ return _serverName;	}

const std::string&	serverConfig::errorPages(int errorCode)	{ return _errorPages[errorCode]; }