#pragma once

#include <map>
#include <vector>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "conf.hpp"
#include "request.hpp"
#include "response.hpp"