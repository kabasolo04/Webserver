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

std::string getMimeType(const std::string &path)
{
	// Extract extension
	std::string ext;
	size_t pos = path.find_last_of('.');
	if (pos != std::string::npos)
		ext = path.substr(pos + 1);

	// Lowercase extension safely
	for (size_t i = 0; i < ext.size(); ++i)
		ext[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(ext[i])));

	// MIME lookup table
	static const std::map<std::string, std::string> mime;
	// We must initialize it in a function-local static block in C++98
	if (mime.empty())
	{
		// non-const reference via const_cast (C++98 trick)
		std::map<std::string, std::string> &m = const_cast<std::map<std::string, std::string>&>(mime);
		
		m["jpg"] =  "image/jpeg";
		m["jpeg"] = "image/jpeg";
		m["png"] =  "image/png";
		m["gif"] =  "image/gif";
		m["bmp"] =  "image/bmp";
		m["svg"] =  "image/svg+xml";

		m["html"] = "text/html";
		m["htm"]  = "text/html";
		m["php"]  = "text/html";
		m["css"]  = "text/css";
		m["js"]   = "application/javascript";
		m["txt"]  = "text/plain";
		m["md"]   = "text/plain";
		m["ini"]  = "text/plain";
		m["log"]  = "text/plain";

		m["c"]    = "text/x-c";
		m["cpp"]  = "text/x-c";
		m["h"]    = "text/plain";

		m["pdf"]  = "application/pdf";
		m["doc"]  = "application/msword";
		m["docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
		m["xls"]  = "application/vnd.ms-excel";
		m["xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
		m["ppt"]  = "application/vnd.ms-powerpoint";

		m["zip"]  = "application/zip";
		m["rar"]  = "application/vnd.rar";
		m["tar"]  = "application/x-tar";
		m["gz"]   = "application/gzip";
		m["7z"]   = "application/x-7z-compressed";

		m["mp3"]  = "audio/mpeg";
		m["wav"]  = "audio/wav";
		m["flac"] = "audio/flac";
		m["aac"]  = "audio/aac";

		m["mp4"]  = "video/mp4";
		m["avi"]  = "video/x-msvideo";
		m["mkv"]  = "video/x-matroska";
		m["mov"]  = "video/quicktime";
	}

	// Look up extension
	std::map<std::string, std::string>::const_iterator it = mime.find(ext);
	if (it != mime.end())
		return it->second;

	// Unknown, but file exists → binary
	std::ifstream test(path.c_str());
	if (test.good())
		return "application/octet-stream";

	// Not found
	return "";
}

static StatusCode saveFile(const std::string &part, location *loc)
{
	size_t sep = part.find("\r\n\r\n");
	if (sep == std::string::npos)
		return BAD_REQUEST;
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
		mkdir((loc->getRoot() + "/" + loc->getUploadStore()).c_str(), 0755);
		std::string filename = loc->getRoot() + "/" + loc->getUploadStore() + "/" + headers.substr(q1 + 1, q2 - q1 - 1);

		std::ofstream out(filename.c_str(), std::ios::binary);

		if (out)
		{
			out.write(content.data(), content.size());
			out.close();
			std::cout << "Saved file: " << filename << std::endl;
			return CREATED;
		}
		if (!out.is_open())
		{
			std::cerr << "Failed to open: " << filename << std::endl;
			return INTERNAL_SERVER_ERROR;
		}
	}
	return BAD_REQUEST;
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
	return CREATED;
}

//---------------------------------------------------------------------------//
// GET                                                                       //
//---------------------------------------------------------------------------//

StatusCode	request::setUpGet()
{
	std::ifstream	file;

	std::cout << "IM GET" << std::endl;

	setQuery();	// Strip the query from the path to separate them

	if (is_directory(_path))
	{
		if (!_location.isAutoindex())
			_path += "/" + _location.getIndex();
		else
			return AUTOINDEX;
	}
	if (!is_file(_path))
		return NOT_FOUND;

	_contentType = getMimeType(_path);
	_infile = open(_path.c_str(), O_RDONLY);
	if (_infile < 0)
		return NOT_FOUND;
	std::cout << _path << " was found by getsetup" << std::endl;

	std::cout << "FILE FOUND AND OPENED" << std::endl;
	
	return OK;
}

void	request::setQuery()
{
	size_t mark = _path.find("?");
	if (mark == std::string::npos)
		_query = "";
	_query = _path.substr(mark + 1);
	_path = _path.substr(0, mark);
}

//---------------------------------------------------------------------------//
// POST                                                                      //
//---------------------------------------------------------------------------//

enum ContentType
{
	CT_MULTIPART,
	CT_FORM,
	CT_JSON,
	CT_UNSUPPORTED
};

static ContentType	contentType(const std::string& ctype)
{
	if (ctype.find("multipart/form-data") != std::string::npos)
		return CT_MULTIPART;
	if (ctype.find("application/x-www-form-urlencoded") != std::string::npos)
		return CT_FORM;
	if (ctype.find("application/json") != std::string::npos)
		return CT_JSON;
	return CT_UNSUPPORTED;
}

StatusCode	request::setUpPost()
{
	StatusCode code;

	if (_headers.find("Content-Type") == _headers.end())
		return BAD_REQUEST;

	switch (contentType(_headers["Content-Type"]))
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


/* StatusCode request::handleMultipart()
{
	printHeaders();
	std::string contentType = _headers["content-type"];
	size_t p = contentType.find("boundary=");
	if (p == std::string::npos)
		return BAD_REQUEST;

	size_t boundaryStart = p + 9;
	if (boundaryStart >= contentType.size())
		return BAD_REQUEST;	// Malformed header

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
 */

StatusCode request::handleMultipart()
{
	std::string contentType = _headers["Content-Type"];
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

StatusCode	request::setUpDel()
{
	if (is_directory(_path))
		return FORBIDEN;

	if (std::remove(_path.c_str()) == 0)
	{
		_body = "<html><body><h1>" + _path + " deleted successfully!</h1></body></html>";
		return OK;
	}

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
