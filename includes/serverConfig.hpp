#pragma once

#include "WebServer.hpp"

struct listenEntry
{
	std::string			_host;		// e.g. "127.0.0.1" or "0.0.0.0"
	int					_port;		// e.g. 8080
	
	int					_fd;		// Listening socket for this host:port

	listenEntry(): _host("0.0.0.0"), _fd(-1) {}
};

class serverConfig
{
	private:
		std::vector<listenEntry>	_listen;		// ["127.0.0.1:8080", "192.168.1.10:8080"]
		std::string					_serverName;
		location					_default;		// Server config variables
		std::vector<location>		_locations;

		int		setLocation(const std::vector<std::string>& tokens, int i);
		void	setListen(TOKEN_IT& it);
		void	setServerName(TOKEN_IT& it);
		void	setLocation(TOKEN_IT& it, TOKEN_IT& end);

	public:
		serverConfig();
		serverConfig(const serverConfig& other);
		~serverConfig();
		
		serverConfig& operator=(const serverConfig& other);
		
		std::vector<listenEntry>::iterator	listenBegin();
		std::vector<listenEntry>::iterator	listenEnd();

		serverConfig& parseServer(TOKEN_IT& it, TOKEN_IT& end);

		location&	getDefaultLocation();
		location&	getLocation(std::string path);
		const std::string&	serverName();
};

bool setNonBlocking(int fd);

/*

struct LocationConfig
{
	std::string							_path;
	std::string							_root;
	std::vector<std::string>			_methods;
	std::string							_index;
	bool								_autoindex;

	bool								_uploadEnable;
	std::string							_uploadStore;

	std::map<std::string, std::string>	_cgiExtensions; // ".php" -> "/usr/bin/php-cgi"
	std::string							_cgiRoot;

	std::string							_redirect;		// for return 301 ...
};

class serverConfig
{
	public:
		std::vector<listenEntry>	_listen;				// ["127.0.0.1:8080", "192.168.1.10:8080"]
		std::string 				_serverName;
		std::string					_root;					// Default root for all locations
		std::string					_index;
		size_t						_bodySize;
		std::map<int, std::string> 	_errorPages;			// 404 -> "/errors/404.html"
		//std::vector<LocationConfig>	_locations;

		serverConfig();
		~serverConfig();

		std::vector<listenEntry>::iterator	listenBegin();
		std::vector<listenEntry>::iterator	listenEnd();

		const std::string&	serverName();
		const std::string&	root();
		const size_t&		bodySize();
		const std::string&	errorPages(int errorCode);

};

*/