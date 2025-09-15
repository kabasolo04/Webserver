#pragma once

#include "WebServer.hpp"

bool			is_directory(const std::string &path);
bool			is_file(const std::string &path);
void			setNonBlocking(int fd);
std::string		getReasonPhrase(StatusCode code);
std::string		buildResponse(StatusCode code, const std::string& body, const std::string& contentType);
std::string		getMimeType(const std::string &path);