#include "WebServer.hpp"

bool	is_directory(const std::string &path)
{
	struct stat info;
	if (stat(path.c_str(), &info) != 0)
		return false;			  // failed to get info (doesn't exist, etc.)
	return S_ISDIR(info.st_mode); // true if directory
}

bool	is_file(const std::string &path)
{
	struct stat info;
	if (stat(path.c_str(), &info) != 0)
		return false;
	return S_ISREG(info.st_mode); // true if regular file
}

void	setNonBlocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;

	if (old_option == -1)
		throw std::runtime_error("fctnl() failed | utils.cpp - setNonBlocking()");
	if (fcntl(fd, F_SETFL, new_option) == -1)
		throw std::runtime_error("fctnl() failed | utils.cpp - setNonBlocking()");
}

std::string getReasonPhrase(StatusCode code)
{
	switch (code)
	{
		case OK: return "OK";
		case FOUND: return "Found";
		case BAD_REQUEST: return "Bad Request";
		case NOT_FOUND: return "Not Found";
		case INTERNAL_SERVER_ERROR: return "Internal Server Error";
		case FORBIDEN: return "Forbiden";
		case METHOD_NOT_ALLOWED: return "Method Not Allowed";
		case PAYLOAD_TOO_LARGE: return "Payload Too Large";
		case UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
		case LOL: return "No Fucking Idea Mate";
		default: return "Unknown";
	}
}

std::string buildResponse(StatusCode code, const std::string& body, const std::string& contentType)
{
	std::ostringstream oss;
	oss << "HTTP/1.1 " << code << " " << getReasonPhrase(code) << "\r\n";
	oss << "Content-Type: " << contentType << "\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";
	oss << "Connection: close\r\n";
	oss << "\r\n";
	
	std::string response = oss.str();
	response.append(body);
	return response;
}

std::string getMimeType(const std::string &path)
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
