#pragma once

#include "WebServer.hpp"

class location
{
	private:
		std::string							_path;
		std::string							_root;
		std::string							_index;
		std::vector<std::string>			_methods;
		bool								_autoindex;

		std::map<int, std::string> 			_errorPages;

		size_t								_headerSize;
		size_t								_bodySize;

		bool								_uploadEnable;
		std::string							_uploadStore;

		std::string							_cgiRoot;
		std::map<std::string, std::string>	_cgiExtensions;

		int	setPath(const std::vector<std::string>& tokens, int i);
		int	setRoot(const std::vector<std::string>& tokens, int i);
		int	setIndex(const std::vector<std::string>& tokens, int i);
		int	addMethods(const std::vector<std::string>& tokens, int i);
		int	setAutoindex(const std::vector<std::string>& tokens, int i);

		int	addErrorPage(const std::vector<std::string>& tokens, int i);

		int	setHeaderSize(const std::vector<std::string>& tokens, int i);
		int	setBodySize(const std::vector<std::string>& tokens, int i);

		int	setUploadEnable(const std::vector<std::string>& tokens, int i);
		int	setUploadStore(const std::vector<std::string>& tokens, int i);

		int	setCgiRoot(const std::vector<std::string>& tokens, int i);
		int	addCgiExtension(const std::vector<std::string>& tokens, int i);


	public:
		location();
		location(const location& _default);
		~location();

		int handleDirective(const std::string& key, const std::vector<std::string>& tokens, size_t i);

		const std::string& 				getPath() const;	
		const std::string& 				getRoot() const;
		const std::string& 				getIndex() const;
		const std::vector<std::string>&	getMethods() const;
		bool							isAutoindex() const;

		const std::map<int, std::string>& 	getErrorPages() const;
		
		size_t	getBodySize() const;
		size_t	getHeaderSize() const;
		
		bool	isUploadEnabled() const;
		const	std::string& getUploadStore() const;
		
		const	std::string& getCgiRoot() const;
		const	std::map<std::string, std::string>& getCgiExtensions() const;
};
