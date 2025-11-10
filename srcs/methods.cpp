#include "WebServer.hpp"

//---------------------------------------------------------------------------//
// UTILS                                                                       //
//---------------------------------------------------------------------------//

static bool	is_directory(const std::string &path)
{
	struct stat info;
	if (stat(path.c_str(), &info) != 0)
		return false;			  // failed to get info (doesn't exist, etc.)
	return S_ISDIR(info.st_mode); // true if directory
}

static bool	is_file(const std::string &path)
{
	struct stat info;
	if (stat(path.c_str(), &info) != 0)
		return false;
	return S_ISREG(info.st_mode); // true if regular file
}

static std::string	getMimeType(const std::string &path)
{
	size_t dot = path.find_last_of('.');
	if (dot == std::string::npos) return "application/octet-stream";
	std::string ext = path.substr(dot + 1);

	if (ext == "html" || ext == "htm") return "text/html";
	if (ext == "css") return "text/css";
	if (ext == "js") return "application/javascript";
	if (ext == "png") return "image/png";
	if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
	if (ext == "gif") return "image/gif";
	if (ext == "json") return "application/json";

	return "application/octet-stream";
}

static void saveFile(const std::string &part, location *loc)
{
		size_t sep = part.find("\r\n\r\n");
	if (sep == std::string::npos)
		return;
	std::string headers = part.substr(0, sep);
	std::string content = part.substr(sep + 4);

	// Trim trailing CRLF
	if (content.size() >= 2 && content.substr(content.size() - 2) == "\r\n")
		content.erase(content.size() - 2);

	// Extract filename
	size_t fnPos = headers.find("filename");
	if (fnPos != std::string::npos)
	{
		size_t q1 = headers.find("\"", fnPos);
		size_t q2 = headers.find("\"", q1 + 1);
		std::string filename = loc->getRoot() + "/" + headers.substr(q1 + 1, q2 - q1 - 1);

		std::ofstream out(filename.c_str(), std::ios::binary);
		if (out)
		{
			out.write(content.data(), content.size());
			out.close();
			std::cout << "Saved file: " << filename << std::endl;
		}
	}
}

static void	saveForm(const std::string &part, location *loc)
{
	(void)part;
	struct timeval tp;
	gettimeofday(&tp, NULL);
	long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	std::stringstream ss;
	ss << ms;
	std::string time = ss.str();
 
	std::cout << "BODY:\n" << part << "\n-------------" << std::endl;

	mkdir((loc->getRoot() + "/forms").c_str(), 0755);
	std::string filename = loc->getRoot() + "/forms/" + time + ".txt";
		std::ofstream out(filename.c_str(), std::ios::binary);
	if (!out)
		throw httpResponse(INTERNAL_SERVER_ERROR);
	out.write(part.data(), part.size());
	out.close();
	std::cout << "Saved form file: " << filename << std::endl;
}

//---------------------------------------------------------------------------//
// GET                                                                       //
//---------------------------------------------------------------------------//

myGet::myGet(request* baby): request(*baby) {}

myGet::~myGet() {}

StatusCode	myGet::setUpMethod()
{
	std::ifstream file;

	//setQuery();	// Strip the query from the path to separate them

	if (is_directory(_path))
	{
		if (!_location.isAutoindex())
			_path += "/" + _location.getIndex();
		else
			generateAutoIndex();
	}

	if (!is_file(_path))
	{
		if (!_location.isAutoindex())
			return NOT_FOUND;
		//else
		//	return generateAutoIndex();
	}

	//if (isCgiScript(_path) != "")
	//{
	//	cgi("/usr/bin/php-cgi");		// adjust interpreter
	//	_contentType = "text/html";		// or parse CGI headers if needed
	//}

	//if (isCgiScript(_path) != "")
	//	return(cgi(isCgiScript(_path)), throw httpResponse(this));

	_infile = open(_path.c_str(), O_RDONLY);
	if (_infile < 0)
		return NOT_FOUND;

	//setNonBlocking(_infile);

	std::ostringstream header;
	header << "HTTP/1.1 200 OK\r\n"
		<< "Content-Type: text/html\r\n"
		<< "Transfer-Encoding: chunked\r\n"
		<< "\r\n";

	write(_fd, header.str().c_str(), header.str().size());

	_buffer.clear();

	return FINISHED;
}

StatusCode myGet::processMethod()
{
	return readAndSend();
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
}

//---------------------------------------------------------------------------//
// POST                                                                      //
//---------------------------------------------------------------------------//

myPost::myPost(request* baby): request(*baby) {}

myPost::~myPost() {}

StatusCode	myPost::setUpMethod()
{
	return FINISHED;
}

StatusCode myPost::processMethod()
{
	if (isCgiScript(_path) != "")
		return (cgi(isCgiScript(_path)), FINISHED);
	if (_headers.find("content-type") == _headers.end())
		throw httpResponse(BAD_REQUEST);

	const std::string &ctype = _headers["content-type"];
	if (ctype.find("multipart/form-data") != std::string::npos)
		handleMultipart();
	else if (ctype.find("application/x-www-form-urlencoded") != std::string::npos)
		saveForm(_body, &_location);
	else if (ctype.find("application/json") != std::string::npos)
		saveForm(_body, &_location);
	else
		return UNSUPPORTED_MEDIA_TYPE;
	_body = "<html><body><h1>Upload successful!</h1></body></html>";
	return FINISHED;
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

	while (start < _body.size())
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

		if (last)
			break;

		std::string part = _body.substr(start, next - start);
		if (part.find("filename=") != std::string::npos)
			saveFile(part, &_location);
		else
			saveForm(part, &_location);

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

StatusCode	myDelete::setUpMethod()
{
	return FINISHED;
}

StatusCode	myDelete::processMethod()
{
	if (is_directory(_path))
		return FORBIDEN;
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
				return NOT_FOUND;
			case EACCES: // Permission denied
				return FORBIDEN;
			case EPERM:  // Operation not permitted
				return FORBIDEN;
			default:     // Something else went wrong
				return INTERNAL_SERVER_ERROR;
		}
	}
	return FINISHED;
}
