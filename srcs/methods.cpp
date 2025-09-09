#include "WebServer.hpp"

//---------------------------------------------------------------------------//
// GET                                                                       //
//---------------------------------------------------------------------------//

myGet::myGet(int fd, std::string buffer) : request(fd, buffer) {}

myGet::~myGet() {}

void myGet::brain(const std::string &status_n_msg, std::ifstream &file)
{
	std::string line;
	std::string buffer;
	std::ostringstream oss;

	buffer = "";
	while (std::getline(file, line))
		buffer += line;
	oss << "HTTP/1.1 " << status_n_msg << " \r\n";
	oss << "Content-Type: text/html\r\n";
	oss << "Content-Length: " << buffer.size() << "\r\n";
	oss << "Connection: close\r\n";
	oss << "\r\n";
	oss << buffer;	
	std::string response = oss.str();
	write(_fd, response.c_str(), response.size());
}

void	myGet::doTheThing()
{
	std::ifstream file;
	std::string fullPath = "./www" + _path;

	if (is_directory(fullPath))
		fullPath += "/index.html";

	if (is_file(fullPath))
	{
		file.open(fullPath.c_str());
		if (file.is_open())
			brain("200 OK", file);
		else
		{
			file.open("./www/404.html");
			brain("404 NOT FOUND", file);
		}
	}
	else
	{
		file.open("./www/404.html");
		brain("404 NOT FOUND", file);
	}
}

bool myGet::makeTheCheck()
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

void myPost::doTheThing()
{
	std::cout << _buffer << std::endl;
}

bool myPost::makeTheCheck()
{
	if (!_headerCheck)
	{
		std::cout << "no emo leido" << std::endl;
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
		{
			std::cout << "GITANOOOOOOOO" << std::endl;
			// throw
		}
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

void myDelete::doTheThing()
{
}

bool myDelete::makeTheCheck()
{
	if (_buffer.find("\r\n\r\n") != std::string::npos)
	{
		return (getHeaderVars(), 1);
	}
	return 0;
}
