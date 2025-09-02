#include "WebServer.hpp"

//---------------------------------------------------------------------------//
// GET                                                                       //
//---------------------------------------------------------------------------//

myGet::myGet(int fd, std::string buffer): request(fd, buffer) {}

myGet::~myGet() {}

void myGet::doTheThing()
{
	std::string html = "<!DOCTYPE html>\n"
					   "<html>\n"
					   "<head><title>Hello</title></head>\n"
					   "<body><h1>Hello, World!</h1></body>\n"
					   "</html>";

	std::ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\n";
	oss << "Content-Type: text/html\r\n";
	oss << "Content-Length: " << html.size() << "\r\n";
	oss << "Connection: close\r\n";
	oss << "\r\n";
	oss << html;

	std::string request = oss.str();
	write(_fd, request.c_str(), request.size());
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
