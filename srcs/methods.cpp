#include "WebServer.hpp"

//---------------------------------------------------------------------------//
// GET                                                                       //
//---------------------------------------------------------------------------//

myGet::myGet(int fd, std::string buffer) : request(fd, buffer) {}

myGet::~myGet() {}

void myGet::response(std::ifstream &file)
{
	std::string line;
	std::string buffer;
	std::ostringstream oss;

	this->printHeaders();

	oss << file.rdbuf(); // reads raw bytes into oss
	buffer = oss.str();
	std::string response = buildResponse(OK, buffer, getMimeType(_path));
	write(_fd, response.c_str(), response.size());
}

void	myGet::process()
{
	std::ifstream file;
	_path = conf::root() + _path;

	if (is_directory(_path))
		_path += "/index.html";

	if (is_file(_path))
	{
		file.open(_path.c_str());
		if (file.is_open())
			response(file);
		else
			throw httpException(NOT_FOUND);
	}
	else
		throw httpException(NOT_FOUND);
}

bool myGet::check()
{
	if (_buffer.find("\r\n\r\n") != std::string::npos)
	{
		return (getHeaderVars(), 1);
	}
	return 0;
}

//---------------------------------------------------------------------------//
// POST                                                                      //
//---------------------------------------------------------------------------//
	
myPost::myPost(int fd, std::string buffer) : request(fd, buffer), _headerCheck(0) {}

myPost::~myPost() {}

void myPost::process()
{
	std::cout << _buffer << std::endl;
}

bool myPost::check()
{
	if (!_headerCheck)
	{
		const long unsigned int it = _buffer.find("\r\n\r\n");
		if (it != std::string::npos)
		{
			getHeaderVars();
			_headerCheck = 1;
			_buffer.erase(0, it + 4);
		}
	}
	else
	{
		if (_headers.find("Content-Length") == _headers.end())
			throw httpException(BAD_REQUEST);
		else
		{
			std::stringstream ss(_headers["Content-Length"]);
			long unsigned int len = 0;

			ss >> len;

			return (_buffer.length() >= len);
		}
	}
	return (0);
}

//---------------------------------------------------------------------------//
// DELETE                                                                    //
//---------------------------------------------------------------------------//

myDelete::myDelete(int fd, std::string buffer) : request(fd, buffer) {}

myDelete::~myDelete() {}

void myDelete::process()
{
}

bool myDelete::check()
{
	if (_buffer.find("\r\n\r\n") != std::string::npos)
	{
		return (getHeaderVars(), 1);
	}
	return 0;
}
