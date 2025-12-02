#include "WebServer.hpp"

location::location():	// Deafult defined vasr
	//root(empty),
	_index("index.html"),
	//methods(empty)
	_autoindex(0),
	//errorPages(empty)
	_requestLineSize(256),
	_headerSize(1024),
	_bodySize(1048576),
	_uploadEnable(0),
	_uploadStore("uploads")
	//cgiExtensions(empty)
	//cgiRoot(empty)
	{
		_cgiExtensions["php"] = "/usr/bin/php-cgi";
		_cgiExtensions["py"] = "/usr/bin/python3";
	}

location::location(const location& _default):	// Copy constructor for containers
	_path				(_default._path				),
	_root				(_default._root				),
	_index				(_default._index			),
	_methods			(_default._methods			),
	_autoindex			(_default._autoindex		),
	_errorPages			(_default._errorPages		),
	_requestLineSize	(_default._requestLineSize	),
	_headerSize			(_default._headerSize		),
	_bodySize			(_default._bodySize			),
	_uploadEnable		(_default._uploadEnable		),
	_uploadStore		(_default._uploadStore		),
	_cgiRoot			(_default._cgiRoot			),
	_cgiExtensions		(_default._cgiExtensions	)
	{}

location::location(const location& _default, int lol):	// Copy constructor for us
	//_path(_default._path),
	_root(_default._root),
	_index(_default._index),
	//methods(empty)
	_autoindex(0),
	//errorPages(empty)
	_requestLineSize(_default._requestLineSize),
	_headerSize(_default._headerSize),
	_bodySize(_default._bodySize),
	_uploadEnable(0)
	//uploadStore(empty)
	//cgiExtensions(empty)
	//cgiRoot(empty)
	{
		(void)lol;
	}

location& location::operator=(const location& other)
{
	if (this != &other)
	{
		_path = other._path;
		_root = other._root;
		_index = other._index;
		_methods = other._methods;
		_autoindex = other._autoindex;

		_errorPages = other._errorPages;

		_requestLineSize = other._requestLineSize;
		_headerSize = other._headerSize;
		_bodySize = other._bodySize;

		_uploadEnable = other._uploadEnable;
		_uploadStore = other._uploadStore;

		_cgiRoot = other._cgiRoot;
		_cgiExtensions = other._cgiExtensions;
	}
	return *this;
}

location::~location() {}

void	location::setRoot			(TOKEN_IT& it, TOKEN_IT& end) { _root = *it;											it++;	(void)end;}
void	location::setIndex			(TOKEN_IT& it, TOKEN_IT& end) { _index = *it;											it++;	(void)end;}
void	location::setAutoindex		(TOKEN_IT& it, TOKEN_IT& end) { _autoindex = (*it == "on" || *it == "1");				it++;	(void)end;}
void	location::setHeaderSize		(TOKEN_IT& it, TOKEN_IT& end) { _headerSize = static_cast<size_t>(atoi((*it).c_str()));	it++;	(void)end;}
void	location::setBodySize		(TOKEN_IT& it, TOKEN_IT& end) { _bodySize = static_cast<size_t>(atoi((*it).c_str()));	it++;	(void)end;}
void	location::setUploadEnable	(TOKEN_IT& it, TOKEN_IT& end) { _uploadEnable = (*it == "on" || *it == "1");			it++;	(void)end;}
void	location::setUploadStore	(TOKEN_IT& it, TOKEN_IT& end) { _uploadStore = *it;										it++;	(void)end;}
void	location::setCgiRoot		(TOKEN_IT& it, TOKEN_IT& end) { _cgiRoot = *it;											it++;	(void)end;}

void	location::addMethods(TOKEN_IT& it, TOKEN_IT& end)	// allow_methods GET POST DELETE;
{
	while (it != end && *it != ";")
	{
		if (*it != "GET" && *it != "POST" && *it != "DELETE")
			throw std::runtime_error("The only supported methods are GET POST and DELETE | location.cpp - addMethods()");
		_methods.push_back(*it);
		it ++;
	}
}

void	location::addErrorPage(TOKEN_IT& it, TOKEN_IT& end)	// error_page 404 /errors/404.html;
{
	int code = atoi((*it).c_str());

	if (++it == end)
		throw std::runtime_error("error_page expects <code> <path> | location.cpp - addMethods()");

	_errorPages[code] = *it++;
}

void	location::addCgiExtension(TOKEN_IT& it, TOKEN_IT& end)	// cgi_extension .php /usr/bin/php-cgi;
{
	std::string ext = *it;
	
	if (++it == end)
		throw std::runtime_error("cgi_extension expects <extension> <binary> | location.cpp | addCgiExtension()");

	_cgiExtensions[ext] = *it++;
}

void	location::setPath(const std::string& path)	{ _path = path;}

struct DirectiveHandler
{
	const char* name;
	void (location::*handler)(TOKEN_IT& it, TOKEN_IT& end);
};

#define HANDLER_SIZE 11

void location::handleDirective(const std::string& key, TOKEN_IT& it, TOKEN_IT& end)
{
	static DirectiveHandler g_directives[HANDLER_SIZE] =
	{
		{"root",					&location::setRoot			},
		{"index",					&location::setIndex			},
		{"autoindex",				&location::setAutoindex		},
		{"header_size",				&location::setHeaderSize	},
		{"client_max_body_size",	&location::setBodySize		},
		{"upload_enable",			&location::setUploadEnable	},
		{"upload_store",			&location::setUploadStore	},
		{"cgi_root",				&location::setCgiRoot		},
		{"allow_methods",			&location::addMethods		},
		{"error_page",				&location::addErrorPage		},
		{"cgi_extension",			&location::addCgiExtension	}
	};

	if (it == end)
		throw std::runtime_error("Directive '" + key + "' expects arguments | location.cpp - handleDirective()");

	for (int i = 0; i < HANDLER_SIZE; i ++)
		if (g_directives[i].name == key)
			return (this->*g_directives[i].handler)(it, end);

	throw std::runtime_error("Unknown directive: '" + key + "' | location.cpp - handleDirective()");
}

const std::string& 				location::getPath() const		{ return _path;		}
const std::string& 				location::getRoot() const		{ return _root;		}
const std::string& 				location::getIndex() const		{ return _index;	}

bool	location::methodAllowed(std::string method) const
{
	std::vector<std::string>::const_iterator it = _methods.begin();

	while (it != _methods.end() && *it != method) it++;

	return (*it == method);
}

bool	location::isAutoindex() const { return _autoindex; }

const std::map<int, std::string>& 	location::getErrorPages() const	{ return _errorPages; }

size_t	location::getRequestLineSize()	const	{ return _requestLineSize;	}
size_t	location::getBodySize()			const	{ return _bodySize;			}
size_t	location::getHeaderSize()		const	{ return _headerSize;		}
		
bool				location::isUploadEnabled() const	{ return _uploadEnable;	}
const std::string&	location::getUploadStore() const	{ return _uploadStore;	}
		
const std::string& 							location::getCgiRoot() const		{ return _cgiRoot;			}
const std::map<std::string, std::string>&	location::getCgiExtensions() const	{ return _cgiExtensions;	}


void location::print() const
{
    std::cout << "----- Location Config -----" << std::endl;

    std::cout << "_path: " << _path << std::endl;
    std::cout << "_root: " << _root << std::endl;
    std::cout << "_index: " << _index << std::endl;

    std::cout << "_methods: ";
    for (size_t i = 0; i < _methods.size(); ++i)
    {
        std::cout << _methods[i];
        if (i + 1 < _methods.size())
            std::cout << ", ";
    }
    std::cout << std::endl;

    std::cout << "_autoindex: " << (_autoindex ? "true" : "false") << std::endl;

    std::cout << "_errorPages:" << std::endl;
    for (std::map<int, std::string>::const_iterator it = _errorPages.begin();
         it != _errorPages.end(); ++it)
    {
        std::cout << "  " << it->first << " -> " << it->second << std::endl;
    }

    std::cout << "_requestLineSize: " << _requestLineSize << std::endl;
    std::cout << "_headerSize: " << _headerSize << std::endl;
    std::cout << "_bodySize: " << _bodySize << std::endl;

    std::cout << "_uploadEnable: " << (_uploadEnable ? "true" : "false") << std::endl;
    std::cout << "_uploadStore: " << _uploadStore << std::endl;

    std::cout << "_cgiRoot: " << _cgiRoot << std::endl;

    std::cout << "_cgiExtensions:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = _cgiExtensions.begin();
         it != _cgiExtensions.end(); ++it)
    {
        std::cout << "  " << it->first << " -> " << it->second << std::endl;
    }

    std::cout << "---------------------------" << std::endl;
}