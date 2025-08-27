#include "WebServer.hpp"

class Iresp //Interface
{
	private:
		int					_fd;
		std::string			_header;
		std::string			_body;

		const std::string	_method;
		const std::string	_path;
		const std::string	_http_version;
		const unsigned int	_status;
		const bool			_allowed;
		
	
		Iresp();
		~Iresp();

	public:
		virtual void	readSocket() = 0;
		virtual void	doTheThing() = 0;
};
