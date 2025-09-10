#pragma once

// C library headers
#include <cerrno>     // for errno
#include <cstdio>     // for printf, FILE
#include <cstdlib>    // for exit, malloc
#include <cstring>    // for memset, strerror

// C++ standard library headers
#include <iostream>   // for std::cout, std::cerr
#include <vector>     // for std::vector
#include <map>        // for std::map
#include <algorithm>  // for std::sort, std::find
#include <string>     // for std::string

// POSIX / system headers
#include <fcntl.h>        // for open, O_* flags, fcntl
#include <unistd.h>       // for close, read, write, access, chdir
#include <signal.h>       // for signal, kill
#include <sys/wait.h>     // for waitpid
#include <sys/socket.h>   // for socket, bind, listen, accept, connect
#include <sys/epoll.h>    // for epoll_create, epoll_ctl, epoll_wait
#include <netinet/in.h>   // for sockaddr_in, htons, htonl
#include <arpa/inet.h>    // for inet_addr, ntohl, ntohs
#include <netdb.h>        // for getaddrinfo, freeaddrinfo
#include <sys/stat.h>     // for stat
#include <dirent.h>       // for opendir, readdir, closedir

#include <sstream> 
#include <fstream>
#include <exception>

#include "conf.hpp"
#include "httpException.hpp"
#include "utils.hpp"
#include "request.hpp"
#include "requestHandler.hpp"
#include "methods.hpp"

//#include "factory.hpp"
