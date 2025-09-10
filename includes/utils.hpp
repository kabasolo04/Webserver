#pragma once

#include "WebServer.hpp"

bool	is_directory(const std::string &path);
bool	is_file(const std::string &path);
int     setNonBlocking(int fd);
