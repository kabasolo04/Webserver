#pragma once

#include "WebServer.hpp"

extern char **g_envp;

bool		isCgiScript(std::string filename);
void		cgi(std::string &responseBody, std::string path, std::string query, std::string command);