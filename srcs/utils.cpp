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
		case NO_CONTENT: return "No Content";
		case FOUND: return "Found";
		case BAD_REQUEST: return "Bad Request";
		case NOT_FOUND: return "Not Found";
		case INTERNAL_SERVER_ERROR: return "Internal Server Error";
		case FORBIDEN: return "Forbiden";
		case METHOD_NOT_ALLOWED: return "Method Not Allowed";
		case PAYLOAD_TOO_LARGE: return "Payload Too Large";
		case UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
		case NOT_IMPLEMENTED: return "Not Implemented";
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

void saveFile(const std::string &part)
{
	size_t sep = part.find("\r\n\r\n");
	if (sep == std::string::npos)
		throw httpResponse(BAD_REQUEST);
	std::string headers = part.substr(0, sep);
	std::string content = part.substr(sep + 4);

	// Trim trailing CRLF
	if (content.size() >= 2 && content.substr(content.size() - 2) == "\r\n")
		content.erase(content.size() - 2);

	// Extract filename
	size_t fnPos = headers.find("filename=");
	if (fnPos == std::string::npos)
		throw httpResponse(BAD_REQUEST);
	size_t q1 = headers.find("\"", fnPos);
	size_t q2 = headers.find("\"", q1 + 1);
	std::string filename = conf::root() + "/" + headers.substr(q1 + 1, q2 - q1 - 1);

	std::ofstream out(filename.c_str(), std::ios::binary);
	if (!out)
		throw httpResponse(INTERNAL_SERVER_ERROR);
	out.write(content.data(), content.size());
	out.close();
	std::cout << "Saved file: " << filename << std::endl;
}

void	saveForm(const std::string &part)
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	std::stringstream ss;
	ss << ms;
	std::string time = ss.str();

	mkdir((conf::root() + "/forms").c_str(), 0755);
	std::string filename = conf::root() + "/forms/" + time + ".txt";
		std::ofstream out(filename.c_str(), std::ios::binary);
	if (!out)
		throw httpResponse(INTERNAL_SERVER_ERROR);
	out.write(part.data(), part.size());
	out.close();
	std::cout << "Saved form file: " << filename << std::endl;
}

std::string	getAbsolutePath(const std::string &path)
{
    char absPath[PATH_MAX];
    if (realpath(path.c_str(), absPath) == NULL)
        throw httpResponse(INTERNAL_SERVER_ERROR);
    return std::string(absPath);
}
