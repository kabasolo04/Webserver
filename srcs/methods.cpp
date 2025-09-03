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

std::string	myGet::getFileToOpen()
{
	std::string	full_request_string = this->_buffer;
	std::string	path = "";
	size_t		i = 0;
	size_t		pos = full_request_string.find(' ');

	if (pos == std::string::npos)
		throw std::runtime_error("Invalid request: no space found");
	while (full_request_string[i] != ' ')
	{
		path += full_request_string[i];
		i++;
	}
	/* std::cout << path << std::endl; */
	return (path);
	/* return ("/"); */
}

void		myGet::doTheThing()
{
	std::string	file_to_open = getFileToOpen();
	std::ifstream	file;
	if (file_to_open == "/")
		file.open("./www/index.html");
	else
		file.open(file_to_open.c_str());

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
