#include "WebServer.hpp"

//---------------------------------------------------------------------------//
// GET                                                                       //
//---------------------------------------------------------------------------//

myGet::myGet(int fd, std::string target, location& loc, std::string reqLine): request(fd, target, loc) { _method = "GET"; _buffer.append(reqLine);}

myGet::~myGet() {}

void myGet::process()
{
	std::ifstream file;

	setQuery();		// Strip the query from the path to separate them

	if (is_directory(_path))
	{
		if (!_location.isAutoindex())
			_path += "/index.html";
		else
			generateAutoIndex();
	}
	if (!is_file(_path))
	{
		if (!_location.isAutoindex())
			throw httpResponse(NOT_FOUND);
		else
			return generateAutoIndex();
	}

	if (isCgiScript(_path) != "")
	{
		cgi(isCgiScript(_path));		// adjust interpreter
		_contentType = "text/html";		// or parse CGI headers if needed
		return (throw httpResponse(this));
	}

	file.open(_path.c_str());
	if (!file.is_open())
		throw httpResponse(NOT_FOUND);

	std::ostringstream oss;

	oss << file.rdbuf(); // reads raw bytes into oss
	_body = oss.str();
	_contentType = getMimeType(_path);

	throw httpResponse(this);
}

void	myGet::setQuery()
{
	size_t mark = _path.find("?");
	if (mark == std::string::npos)
		_query = "";
	_query = _path.substr(mark + 1);
	_path = _path.substr(0, mark);
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

myPost::myPost(int fd, std::string target, location& loc, std::string reqLine): request(fd, target, loc) { _method = "POST"; _buffer.append(reqLine);}

myPost::~myPost() {}

void myPost::process()
{
	if (isCgiScript(_path) != "")
		return (cgi(isCgiScript(_path)), throw httpResponse(this));
	if (_headers.find("Content-Type") == _headers.end())
		throw httpResponse(BAD_REQUEST);

	const std::string &ctype = _headers["Content-Type"];
	if (ctype.find("multipart/form-data") != std::string::npos)
		handleMultipart();
	else if (ctype.find("application/x-www-form-urlencoded") != std::string::npos)
		saveForm(_body, &_location);
	else if (ctype.find("application/json") != std::string::npos)
		saveForm(_body, &_location);
	else
		throw httpResponse(UNSUPPORTED_MEDIA_TYPE);
	_body = "<html><body><h1>Upload successful!</h1></body></html>";
	throw httpResponse(this);
}

bool myPost::readBody()
{
	if (_headers.find("Content-Length") != _headers.end())
	{
		char *endptr = NULL;
		errno = 0;
		unsigned long len = std::strtoul(_headers["Content-Length"].c_str(), &endptr, 10);
		if (errno != 0 || endptr[0] != '\0')
			throw httpResponse(BAD_REQUEST);

		size_t bodyStart = _buffer.find("\r\n\r\n");
		if (bodyStart == std::string::npos)
			return false; // headers not finished yet
		bodyStart += 4; // skip CRLFCRLF
		if (_buffer.size() >= bodyStart + len)
		{
			_body = _buffer.substr(bodyStart, len);
			return true;
		}
		return false; // body not fully read yet
	}
	else if (_headers.find("Transfer-Encoding") != _headers.end()
		&& _headers["Transfer-Encoding"] == "chunked")
		return chunkedCheck();
	else
		throw httpResponse(BAD_REQUEST);
}

void myPost::handleMultipart()
{
	size_t p = _headers["Content-Type"].find("boundary=");
	if (p == std::string::npos)
		throw httpResponse(BAD_REQUEST);
	std::string boundary = "--" + _headers["Content-Type"].substr(p + 9);

	size_t start = _body.find(boundary);
	if (start == std::string::npos)
		throw httpResponse(BAD_REQUEST);
	start += boundary.size() + 2; // skip boundary + CRLF

	while (true)
	{
		size_t next = _body.find(boundary, start);
		bool last = false;
		if (next == std::string::npos)
		{
			next = _body.find(boundary + "--", start);
			if (next == std::string::npos)
				next = _body.size();
			last = true;
		}
		std::string part = _body.substr(start, next - start);
		if (last)
			break;
		if (part.find("filename=") != std::string::npos)
			saveFile(part, &_location);
		else
			saveForm(part, &_location);
		start = next + boundary.size();
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

/*
bool myPost::check()
{
	if (!_headerCheck)
	{
		const long unsigned int it = _buffer.find("\r\n\r\n");
		if (it != std::string::npos)
		{
			setHeaderVars();
			//printHeaders();
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
		else if (_headers.find("Transfer-Encoding") != _headers.end() && _headers["Transfer-Encoding"] == "chunked")
			return(chunkedCheck());
		else
			throw httpResponse(BAD_REQUEST);
	}
	return (0);
}
*/

//---------------------------------------------------------------------------//
// DELETE                                                                    //
//---------------------------------------------------------------------------//

myDelete::myDelete(int fd, std::string target, location& loc, std::string reqLine): request(fd, target, loc) { _method = "DELETE"; _buffer.append(reqLine);}

myDelete::~myDelete() {}

void	myDelete::process()
{
	if (is_directory(_path))
		throw httpResponse(FORBIDEN);
	if (std::remove(_path.c_str()) == 0)
	{
		_body = "<html><body><h1>" + _path + " deleted successfully!</h1></body></html>";
		throw httpResponse(this);
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
