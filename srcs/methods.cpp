#include "WebServer.hpp"

//---------------------------------------------------------------------------//
// GET                                                                       //
//---------------------------------------------------------------------------//

myGet::myGet(int fd, std::string buffer) : request(fd, buffer) {}

myGet::~myGet() {}

void myGet::response(std::ifstream &file)
{
	std::ostringstream oss;

	// this->printHeaders();
	oss << file.rdbuf(); // reads raw bytes into oss
	_body = oss.str();
	_contentType = getMimeType(_path);
}

void myGet::process()
{
	std::ifstream file;

	_path = conf::root() + _path;

	if (is_directory(_path))
	{
		if (!conf::autoindex())
			_path += "/index.html";
		else
			return generateAutoIndex();
	}
		
	if (!is_file(_path))
	{
		if (!conf::autoindex())
			throw httpResponse(NOT_FOUND);
		else
			return generateAutoIndex();
	}

	if (isCgiScript(_path))
	{
		cgi(_body, _path, getQuery(_path), "/usr/bin/php-cgi");	// adjust interpreter
		//_body = responseBody.str();			// assuming File can give back string
		_contentType = "text/html";				// or parse CGI headers if needed
		return;
	}

	file.open(_path.c_str());
	if (!file.is_open())
		throw httpResponse(INTERNAL_SERVER_ERROR);
	response(file);
}

bool myGet::check()
{
	if (_buffer.find("\r\n\r\n") != std::string::npos)
		return (setHeaderVars(), 1);
	return 0;
}

void	myGet::generateAutoIndex()
{
	DIR	*dir;

	if (!_path.empty())
		dir = opendir(_path.c_str());
	else
		dir = opendir(".");
	if (!dir)
		throw httpResponse(INTERNAL_SERVER_ERROR);
	/* ITERATE THROUGH ALL FOLDERS AND FILES AND WRITE THEM IN THE _BODY IN HTML */

	struct dirent* entry;
	while ((entry = readdir(dir)) != (void*)0)
		std::cout << "Archivo o carpeta: " << entry->d_name << std::endl;
	closedir(dir);
}

//---------------------------------------------------------------------------//
// POST                                                                      //
//---------------------------------------------------------------------------//

myPost::myPost(int fd, std::string buffer) : request(fd, buffer), _headerCheck(0) {}

myPost::~myPost() {}

void myPost::process()
{
	if (_headers.find("Content-Type") == _headers.end())
		throw httpResponse(BAD_REQUEST);

	const std::string &ctype = _headers["Content-Type"];
	if (ctype.find("multipart/form-data") != std::string::npos)
		handleMultipart();
	else if (ctype.find("application/x-www-form-urlencoded") != std::string::npos)
		saveForm(_body);
	else if (ctype.find("application/json") != std::string::npos)
		saveForm(_body);
	else
		throw httpResponse(UNSUPPORTED_MEDIA_TYPE);

	std::string body = "<html><body><h1>Upload successful!</h1></body></html>";
	std::string response = buildResponse(OK, body, "text/html");
	write(_fd, response.c_str(), response.size());
}

void myPost::handleMultipart()
{
	std::string boundary;
	size_t pos = _headers["Content-Type"].find("boundary=");
	if (pos != std::string::npos)
		boundary = "--" + _headers["Content-Type"].substr(pos + 9); // prepend --
	else
		throw httpResponse(BAD_REQUEST);

	std::vector<std::string> parts;
	size_t start = 0;
	while (true)
	{
		size_t end = _body.find(boundary, start);
		if (end == std::string::npos)
			break;
		std::string part = _buffer.substr(start, end - start);
		if (!part.empty())
			parts.push_back(part);
		start = end + boundary.size();
	}
	for (std::vector<std::string>::const_iterator it = parts.begin(); it != parts.end(); ++it)
	{
		const std::string &part = *it;
		if (part.find("filename=") != std::string::npos)
			saveFile(part);
		else
			saveForm(part);
	}
}

bool myPost::chunkedCheck()
{
	while (true)
	{
		size_t pos = _buffer.find("\r\n");
		if (pos == std::string::npos)
			return false;

		std::string sizeStr = _buffer.substr(0, pos);
		char *endptr = NULL;
		unsigned long chunkSize = std::strtoul(sizeStr.c_str(), &endptr, 16);
		if (endptr == sizeStr.c_str()) // invalid number
			throw httpResponse(BAD_REQUEST);

		if (chunkSize == 0)
		{
			if (_buffer.size() >= pos + 4)
				return (_buffer.erase(0, pos + 4), 1);
			return false;
		}
		size_t totalNeeded = pos + 2 + chunkSize + 2;
		if (_buffer.size() < totalNeeded)
			return false;
		size_t dataStart = pos + 2;
		_body.append(_buffer, dataStart, chunkSize);
		_buffer.erase(0, dataStart + chunkSize + 2);
	}
}

bool myPost::check()
{
	if (!_headerCheck)
	{
		const long unsigned int it = _buffer.find("\r\n\r\n");
		if (it != std::string::npos)
		{
			setHeaderVars();
			printHeaders();
			_headerCheck = 1;
			_buffer.erase(0, it + 4);
		}
	}
	else
	{
		if (_headers.find("Content-Length") != _headers.end())
		{
			char *endptr = NULL;
			errno = 0;
			unsigned long len = std::strtoul(_headers["Content-Length"].c_str(), &endptr, 10);
			if (errno != 0 || endptr[0] != '\0')
				throw httpResponse(BAD_REQUEST);
			if (_buffer.size() >= len)
			{
				_body = _buffer.substr(0, len);
				return true;
			}
		}
		// Chunked transfer
		else if (_headers.find("Transfer-Encoding") != _headers.end() &&
				_headers["Transfer-Encoding"] == "chunked")
			return(chunkedCheck());
		else
			throw httpResponse(BAD_REQUEST);
	}
	return (0);
}

//---------------------------------------------------------------------------//
// DELETE                                                                    //
//---------------------------------------------------------------------------//

myDelete::myDelete(int fd, std::string buffer) : request(fd, buffer) {}

myDelete::~myDelete() {}

void	myDelete::process()
{
	_path = conf::root() + _path;
	if (is_directory(_path))
		throw httpResponse(FORBIDEN);
	if (std::remove(_path.c_str()) == 0)
	{
		std::string body = "<html><body><h1>" + _path + " deleted successfully!</h1></body></html>";
		std::string response = buildResponse(OK, body, "text/html");
		write(_fd, response.c_str(), response.size());
	}
	else
	{
		switch (errno)
		{
			case ENOENT: // File doesn't exist
				throw httpResponse(NOT_FOUND);
			case EACCES: // Permission denied
				throw httpResponse(FORBIDEN);
			case EPERM:  // Operation not permitted
				throw httpResponse(FORBIDEN);
			default:     // Something else went wrong
				throw httpResponse(INTERNAL_SERVER_ERROR);
		}
	}
}

bool myDelete::check()
{
	if (_buffer.find("\r\n\r\n") != std::string::npos)
		return (setHeaderVars(), 1);
	return 0;
}
