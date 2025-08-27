#pragma once
#include "WebServer.hpp"

class conf
{
	private:
		static int						_port;
		static std::string				_serverName;
		static std::string				_index;
		static std::string				_root;
		static std::vector<std::string>	_methods;

		conf();
	
	protected:
		void	setConfig(char* filename);

	public:
		const int&			port();
		const std::string&	serverName();
		const std::string&	index();
		const std::string&	root();
};