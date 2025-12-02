#pragma once

#include "WebServer.hpp"

struct data
{
	std::string							_path;
	std::string							_root;
	std::string							_index;
	std::vector<std::string>			_methods;
	bool								_autoindex;
	std::map<int, std::string> 			_errorPages;
	size_t								_requestLineSize; // Isnt Parsed
	size_t								_headerSize;
	size_t								_bodySize;
	bool								_uploadEnable;
	std::string							_uploadStore;
	std::string							_cgiRoot;
	std::map<std::string, std::string>	_cgiExtensions;
};

class location
{
	private:
		struct								_data;

		std::string							_path;

		std::string							_root;
		std::string							_index;
		std::vector<std::string>			_methods;
		bool								_autoindex;

		std::map<int, std::string> 			_errorPages;

		size_t								_requestLineSize; // Isnt Parsed
		size_t								_headerSize;
		size_t								_bodySize;

		bool								_uploadEnable;
		std::string							_uploadStore;

		std::string							_cgiRoot;
		std::map<std::string, std::string>	_cgiExtensions;

		void	setRoot			(TOKEN_IT& it, TOKEN_IT& end);
		void	setIndex		(TOKEN_IT& it, TOKEN_IT& end);
		void	addMethods		(TOKEN_IT& it, TOKEN_IT& end);
		void	setAutoindex	(TOKEN_IT& it, TOKEN_IT& end);

		void	addErrorPage	(TOKEN_IT& it, TOKEN_IT& end);

		void	setHeaderSize	(TOKEN_IT& it, TOKEN_IT& end);
		void	setBodySize		(TOKEN_IT& it, TOKEN_IT& end);

		void	setUploadEnable	(TOKEN_IT& it, TOKEN_IT& end);
		void	setUploadStore	(TOKEN_IT& it, TOKEN_IT& end);

		void	setCgiRoot		(TOKEN_IT& it, TOKEN_IT& end);
		void	addCgiExtension	(TOKEN_IT& it, TOKEN_IT& end);

	public:
		location();
		location(const location& _default);				// Copy constructor for the containers
		location(const location& _default, int lol);	// Copy constructor for us, the int is just a flag
		location& operator=(const location& other);
		~location();
	
		void	setPath(const std::string& path);
		void 	handleDirective(const std::string& key, TOKEN_IT& it, TOKEN_IT& end);

		// GETTERS
		
		const std::string& 				getPath() const;
		const std::string& 				getRoot() const;
		const std::string& 				getIndex() const;
		bool							methodAllowed(std::string method) const;
		bool							isAutoindex() const;

		const std::map<int, std::string>& 	getErrorPages() const;
		
		size_t	getRequestLineSize() const;
		size_t	getBodySize() const;
		size_t	getHeaderSize() const;
		
		bool	isUploadEnabled() const;
		const	std::string& getUploadStore() const;
		
		const	std::string& getCgiRoot() const;
		const	std::map<std::string, std::string>& getCgiExtensions() const;
		void print() const;

};
