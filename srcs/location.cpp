#include "WebServer.hpp"

location::location():
	//path(empty)
	//root(empty),
	//index(empty),
	//methods(empty)
	_autoindex(0),
	//errorPages(empty)
	_headerSize(0),
	_bodySize(0),
	_uploadEnable(0)
	//uploadStore(empty)
	//cgiExtensions(empty)
	//cgiRoot(empty)
	{}

location::location(const location& _default):
	//path(empty)
	_root(_default._root),
	_index(_default._index),
	//methods(empty)
	_autoindex(0),
	//errorPages(empty)
	_headerSize(0),
	_bodySize(0),
	_uploadEnable(0)
	//uploadStore(empty)
	//cgiExtensions(empty)
	//cgiRoot(empty)
	{}

location::~location() {}

#define TOKENS const std::vector<std::string>& tokens, int i

int	location::setPath(TOKENS)			{ _path = tokens[i];											return (1); }
int	location::setRoot(TOKENS)			{ _root = tokens[i];											return (1); }
int	location::setIndex(TOKENS)			{ _index = tokens[i];											return (1); }
int	location::setAutoindex(TOKENS)		{ _autoindex = (tokens[i] == "on" || tokens[i] == "1");			return (1);	}
int	location::setHeaderSize(TOKENS)		{ _headerSize = static_cast<size_t>(atoi(tokens[i].c_str()));	return (1); }
int	location::setBodySize(TOKENS)		{ _bodySize = static_cast<size_t>(atoi(tokens[i].c_str()));		return (1);	}
int	location::setUploadEnable(TOKENS)	{ _uploadEnable = (tokens[i] == "on" || tokens[i] == "1");		return (1);	}
int	location::setUploadStore(TOKENS)	{ _uploadStore = tokens[i];										return (1);	}
int	location::setCgiRoot(TOKENS)		{ _cgiRoot = tokens[i];											return (1); }

int	location::addMethods(TOKENS)
{

}

int	location::addErrorPage(TOKENS)
{

}

int	location::addCgiExtension(TOKENS)
{
	
}

struct DirectiveHandler
{
    const char* name;
    int (location::*handler)(TOKENS);
};

#define HANDLER_SIZE 12

int location::handleDirective(const std::string& key, const std::vector<std::string>& tokens, size_t i)
{
	static DirectiveHandler g_directives[HANDLER_SIZE] =
	{
		{"path",					&location::setPath},
		{"root",					&location::setRoot},
		{"index",					&location::setIndex},
		{"autoindex",				&location::setAutoindex},
		{"header_size",				&location::setHeaderSize},
		{"client_max_body_size",	&location::setBodySize},
		{"upload_enable",			&location::setUploadEnable},
		{"upload_store",			&location::setUploadStore},
		{"cgi_root",				&location::setCgiRoot},
		{"allow_methods",			&location::addMethods},
		{"error_page",				&location::addErrorPage},
		{"cgi_extension",			&location::addCgiExtension}
	};

	if (tokens.size() < 2 + i)
		throw std::runtime_error("directive '" + key + "' expects arguments | location.cpp - handleDirective()");

	for (int j = 0; j < HANDLER_SIZE; j ++)
		if (g_directives[j].name == key)
			return (this->*g_directives[j].handler)(tokens, i);

	throw std::runtime_error("Unknown directive: '" + key + "' | location.cpp - handleDirective()");
}

const std::string& 				location::getPath() const		{ return _path;			}
const std::string& 				location::getRoot() const		{ return _root;			}
const std::string& 				location::getIndex() const		{ return _index;		}
const std::vector<std::string>&	location::getMethods() const	{ return _methods;		}
bool							location::isAutoindex() const	{ return _autoindex;	}
		
const std::map<int, std::string>& 	location::getErrorPages() const	{ return _errorPages; }
		
size_t	location::getBodySize() const	{ return _bodySize;		}
size_t	location::getHeaderSize() const	{ return _headerSize;	}
		
bool				location::isUploadEnabled() const	{ return _uploadEnable; }
const std::string&	location::getUploadStore() const	{ return _uploadStore; }
		
const	std::string& 						location::getCgiRoot() const		{ return _cgiRoot; }
const std::map<std::string, std::string>&	location::getCgiExtensions() const	{ return _cgiExtensions; }