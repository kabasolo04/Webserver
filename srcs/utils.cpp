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

int setNonBlocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;

	if (old_option == -1)
		throw std::runtime_error("fctnl() failed | conf.cpp - setNonBlocking()");
	if (fcntl(fd, F_SETFL, new_option) == -1)
		throw std::runtime_error("fctnl() failed | conf.cpp - setNonBlocking()");
	return 1; 
}
