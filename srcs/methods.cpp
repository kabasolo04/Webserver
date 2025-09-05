#include "WebServer.hpp"

//---------------------------------------------------------------------------//
// GET                                                                       //
//---------------------------------------------------------------------------//

myGet::myGet(int fd, std::string buffer): request(fd, buffer) {}

myGet::~myGet() {}

void		myGet::brain(const std::string& status_n_msg, std::ifstream& file)
{
	std::string		line;
	std::string		buffer;
	std::ostringstream	oss;

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

void		myGet::doTheThing()
{
	getReqLineVars();
	getHeaderVars();
	std::ifstream	file;
	if (_path == "/")
		file.open("./www/index.html");
	else
		file.open(_path.c_str());

	if (!file.is_open())
	{
		file.open("./www/404.html");
		brain("404 NOT FOUND", file);
		return ;
	}
	brain("200 OK", file);
}

bool	myGet::makeTheCheck()
{
	return _buffer.find("\r\n\r\n") != std::string::npos;
}

//---------------------------------------------------------------------------//
// POST                                                                      //
//---------------------------------------------------------------------------//

myPost::myPost(int fd, std::string buffer): request(fd, buffer) {}

myPost::~myPost() {}

void	myPost::doTheThing()
{

}

bool	myPost::makeTheCheck()
{
	return true;
	
}

//---------------------------------------------------------------------------//
// DELETE                                                                    //
//---------------------------------------------------------------------------//

myDelete::myDelete(int fd, std::string buffer): request(fd, buffer) {}

myDelete::~myDelete() {}

void	myDelete::doTheThing()
{

}

bool	myDelete::makeTheCheck()
{
	return _buffer.find("\r\n\r\n") != std::string::npos;
}
