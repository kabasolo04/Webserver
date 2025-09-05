#pragma once
#include "WebServer.hpp"

class conf
{
	private:
		static bool						_autoindex;

		static int						_port;
		static int						_server;
		static int						_bodySize;
		static size_t					_headerSize;

		static std::string				_root;
		static std::string				_host;
		static std::string				_index;
		static std::string				_serverName;

		static std::vector<std::string>	_methods;

		conf();
	
	public:
		static void	setConfig(std::string filename);

		static const bool&			autoindex();

		static const int&			port();
		static const int&			server();
		static const int&			bodySize();
		static const size_t&		headerSize();

		static const std::string&	root();
		static const std::string&	host();
		static const std::string&	index();
		static const std::string&	serverName();

		static bool	methodAllowed(std::string method);
};

int		setNonBlocking(int fd);