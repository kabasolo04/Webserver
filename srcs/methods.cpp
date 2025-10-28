#include "WebServer.hpp"

//---------------------------------------------------------------------------//
// GET                                                                       //
//---------------------------------------------------------------------------//

myGet::myGet(request* baby): request(*baby) {}

myGet::~myGet() {}

void myGet::process()
{
	std::ifstream file;

	setQuery();	// Strip the query from the path to separate them

	if (is_directory(_path))
	{
		if (!_location->isAutoindex())
			_path += "/" + _location->getIndex();
		else
			generateAutoIndex();
	}

	if (!is_file(_path))
	{
		if (!_location->isAutoindex())
			throw httpResponse(NOT_FOUND);
		else
			return generateAutoIndex();
	}

	if (isCgiScript(_path) != "")
		return(cgi(isCgiScript(_path)), throw httpResponse(this));

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
	std::cout << "AUTOINDEEEX" << std::endl;
	throw httpResponse(LOL);
}

//---------------------------------------------------------------------------//
// POST                                                                      //
//---------------------------------------------------------------------------//

myPost::myPost(request* baby): request(*baby) {}

myPost::~myPost() {}

void myPost::process()
{
	if (isCgiScript(_path) != "")
		return (cgi(isCgiScript(_path)), throw httpResponse(this));
	if (_headers.find("content-type") == _headers.end())
		throw httpResponse(BAD_REQUEST);

	const std::string &ctype = _headers["content-type"];
	if (ctype.find("multipart/form-data") != std::string::npos)
		handleMultipart();
	else if (ctype.find("application/x-www-form-urlencoded") != std::string::npos)
		saveForm(_body, _location);
	else if (ctype.find("application/json") != std::string::npos)
		saveForm(_body, _location);
	else
		throw httpResponse(UNSUPPORTED_MEDIA_TYPE);
	_body = "<html><body><h1>Upload successful!</h1></body></html>";
	throw httpResponse(this);
}

void myPost::handleMultipart()
{
	std::string contentType = _headers["content-type"];
	size_t p = contentType.find("boundary=");
	if (p == std::string::npos)
		throw httpResponse(BAD_REQUEST);

	size_t boundaryStart = p + 9; // length of "boundary="
	if (boundaryStart >= contentType.size())
		throw httpResponse(BAD_REQUEST); // malformed header

	std::string boundary = "--" + contentType.substr(boundaryStart);

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

		if (next < start) // sanity check
			throw httpResponse(BAD_REQUEST);

		std::string part = _body.substr(start, next - start);
		if (part.find("filename=") != std::string::npos)
			saveFile(part, _location);
		else
			saveForm(part, _location);

		if (last)
			break;

		start = next + boundary.size();
	}
}


/*
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
*/

//---------------------------------------------------------------------------//
// DELETE                                                                    //
//---------------------------------------------------------------------------//

myDelete::myDelete(request* baby): request(*baby) {}

myDelete::~myDelete() {}

void	myDelete::process()
{
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
