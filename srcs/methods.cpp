#include "methods.hpp"
#include <sstream> 

get::get(int fd): resp(fd) {}

get::~get() {}

bool	get::makeTheCheck(std::string buffer)
{
    return buffer.find("\r\n\r\n") != std::string::npos;
}

void get::doTheThing()
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

    std::string response = oss.str();
    write(_fd, response.c_str(), response.size());
}
