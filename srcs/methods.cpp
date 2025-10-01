#include "WebServer.hpp"

//---------------------------------------------------------------------------//
// GET                                                                       //
//---------------------------------------------------------------------------//

myGet::myGet(request* req, std::map <int, serverConfig*>& servers): request(*req, servers) { }

myGet::~myGet() {}

void myGet::process()
{
	std::ifstream file;

	_path = _server.root() + _path;

	std::cout << "aaaaa" << std::endl;

	if (is_directory(_path))
		_path += "/index.html";

	if (!is_file(_path))
		throw httpResponse(NOT_FOUND);

	file.open(_path.c_str());

	if (!file.is_open())
		throw httpResponse(NOT_FOUND);

	std::ostringstream oss;

	oss << file.rdbuf(); // reads raw bytes into oss
	_body = oss.str();
	_contentType = getMimeType(_path);

	throw httpResponse(this);
}

//---------------------------------------------------------------------------//
// POST                                                                      //
//---------------------------------------------------------------------------//

myPost::myPost(request* req, std::map <int, serverConfig*>& servers): request(*req, servers) { delete req; }

myPost::~myPost() {}

void myPost::process()
{
	if (_headers.find("Content-Type") == _headers.end())
		throw httpResponse(BAD_REQUEST);

	const std::string &ctype = _headers["Content-Type"];
	if (ctype.find("multipart/form-data") != std::string::npos)
		handleMultipart();
	else if (ctype.find("application/x-www-form-urlencoded") != std::string::npos)
		std::cout << "form" << std::endl; // handleFormUrlEncoded();
	else if (ctype.find("application/json") != std::string::npos)
		std::cout << "json" << std::endl; // handleJson();
	else
		throw httpResponse(UNSUPPORTED_MEDIA_TYPE);

	std::string body = "<html><body><h1>Upload successful!</h1></body></html>";
	std::string response = buildResponse(OK, body, "text/html");
	write(_fd, response.c_str(), response.size());
	//throw httpResponse(this);
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

	for (std::vector<std::string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
        const std::string &part = *it;
        if (part.find("filename=") != std::string::npos)
            saveFile(part);
        else
            std::cout << "multipart form received" << std::endl;
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

void myPost::saveFile(const std::string &part)
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
	size_t fnPos = headers.find("filename=");
	if (fnPos != std::string::npos)
	{
		size_t q1 = headers.find("\"", fnPos);
		size_t q2 = headers.find("\"", q1 + 1);
		std::string filename = _server.root() + "/" + headers.substr(q1 + 1, q2 - q1 - 1);

		std::ofstream out(filename.c_str(), std::ios::binary);
		if (out)
		{
			out.write(content.data(), content.size());
			out.close();
			std::cout << "Saved file: " << filename << std::endl;
		}
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

myDelete::myDelete(request* req, std::map <int, serverConfig*>& servers): request(*req, servers) { delete req; }

myDelete::~myDelete() {}

void myDelete::process()
{

}
