#include "WebServer.hpp"

//---------------------------------------------------------------------------//
// UTILS                                                                       //
//---------------------------------------------------------------------------//

//#define FILE		S_ISREG
//#define DIRECTORY	S_ISDIR

//static bool isItA(mode_t type_mask, const std::string &path)
//{
//	struct stat info;
//
//	if (stat(path.c_str(), &info) != 0)
//		return false;
//	return (info.st_mode & S_IFMT) == type_mask;
//}

static bool	is_directory(const std::string &path)
{
	struct stat info;
	if (stat(path.c_str(), &info) != 0)
		return false;			  // failed to get info (doesn't exist, etc.)
	return S_ISDIR(info.st_mode);
}

static bool	is_file(const std::string &path)
{
	struct stat info;
	if (stat(path.c_str(), &info) != 0)
		return false;
	return S_ISREG(info.st_mode); // true if regular file
}

//static std::string	getMimeType(const std::string &path)
//{
//	size_t dot = path.find_last_of('.');
//	if (dot == std::string::npos) return "application/octet-stream";
//	std::string ext = path.substr(dot + 1);
//
//	if (ext == "html" || ext == "htm") return "text/html";
//	if (ext == "css") return "text/css";
//	if (ext == "js") return "application/javascript";
//	if (ext == "png") return "image/png";
//	if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
//	if (ext == "gif") return "image/gif";
//	if (ext == "json") return "application/json";
//
//	return "application/octet-stream";
//}

static StatusCode saveFile(const std::string &part, location *loc)
{
	size_t sep = part.find("\r\n\r\n");
	if (sep == std::string::npos)
		return	BAD_REQUEST;
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
	return FINISHED;
}

static StatusCode	saveForm(const std::string &part, location *loc)
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
		return INTERNAL_SERVER_ERROR;	// Dont know if its okay ********************************************************************************************
	out.write(part.data(), part.size());
	out.close();
	std::cout << "Saved form file: " << filename << std::endl;
	return FINISHED;
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
		else
			generateAutoIndex();
	}

	//if (isCgiScript(_path) != "")
	//{
	//	cgi("/usr/bin/php-cgi");		// adjust interpreter
	//	_contentType = "text/html";		// or parse CGI headers if needed
	//}

	//if (isCgiScript(_path) != "")
	//	return(cgi(isCgiScript(_path)), returnthis));

	_infile = open(_path.c_str(), O_RDONLY);
	if (_infile < 0)
		return NOT_FOUND;

	return OK;
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

enum ContentType
{
	CT_MULTIPART,
	CT_FORM,
	CT_JSON,
	CT_UNSUPPORTED
};

ContentType	contentType(const std::string& ctype)
{
	if (ctype.find("multipart/form-data") != std::string::npos)
		return CT_MULTIPART;
	if (ctype.find("application/x-www-form-urlencoded") != std::string::npos)
		return CT_FORM;
	if (ctype.find("application/json") != std::string::npos)
		return CT_JSON;
	return CT_UNSUPPORTED;
}

StatusCode	myPost::setUpMethod()
{
	//if (isCgiScript(_path) != "")
	//	return (cgi(isCgiScript(_path)), FINISHED);
	if (_headers.find("content-type") == _headers.end())
		return BAD_REQUEST;

	StatusCode code;

	switch (contentType(_headers["content-type"]))
	{
		case CT_MULTIPART:	code = handleMultipart();			break;

		case CT_FORM:		code = saveForm(_body, &_location);	break;

		case CT_JSON:		code = saveForm(_body, &_location);	break;

		default:			return UNSUPPORTED_MEDIA_TYPE;
	}
	
	if (code != FINISHED)
		return code;
	_body = "<html><body><h1>Upload successful!</h1></body></html>";
	return OK;
}

StatusCode myPost::processMethod()
{
	//CGI HEREEEEE
	return FINISHED;
}

StatusCode myPost::handleMultipart()
{
	std::string contentType = _headers["content-type"];
	size_t p = contentType.find("boundary=");
	if (p == std::string::npos)
		return BAD_REQUEST;

	size_t boundaryStart = p + 9;
	if (boundaryStart >= contentType.size())
		return BAD_REQUEST; // malformed header

	std::string boundary = "--" + contentType.substr(boundaryStart);

	size_t start = _body.find(boundary);
	if (start == std::string::npos)
		return BAD_REQUEST;
	start += boundary.size() + 2;

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

		if (next < start)
			return BAD_REQUEST;

		if (last)
			break;

		std::string part = _body.substr(start, next - start);
		if (part.find("filename=") != std::string::npos)
			saveFile(part, &_location);
		else
			saveForm(part, &_location);

		start = next + boundary.size();
	}
	return FINISHED;
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
		returnBAD_REQUEST);
		
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

myDelete::myDelete(request* baby): request(*baby)
{
	_body = "<html><body><h1>" + _path + " deleted successfully!</h1></body></html>";
}

myDelete::~myDelete() {}

StatusCode	myDelete::setUpMethod()
{
	if (is_directory(_path))
		return FORBIDEN;

	if (std::remove(_path.c_str()) == 0)
		return OK;

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

StatusCode	myDelete::processMethod()
{
	return OK;
}
