#include "WebServer.hpp"

std::string request::isCgiScript(std::string filename)
{
	size_t pos = filename.find_last_of(".");
	if (pos == std::string::npos)
		throw httpResponse(BAD_REQUEST);
	std::string extension = filename.substr(pos + 1);
	std::map<std::string, std::string> ext = _location->getCgiExtensions();
	std::map<std::string, std::string>::iterator it = ext.find(extension);
    if (it == ext.end())
        return "";
    return it->second;
/* 	if (extension == "php")
		return "/usr/bin/php-cgi";
	return ""; */
}

static std::string	getAbsolutePath(const std::string &path)
{
	char absPath[PATH_MAX];
	if (realpath(path.c_str(), absPath) == NULL)
		throw httpResponse(INTERNAL_SERVER_ERROR);
	return std::string(absPath);
}


std::vector<char *> buildArgv(const std::string &command, const std::string &path)
{
	std::vector<char *> argv;
	if (command.find("php-cgi") != std::string::npos)
	{
		argv.push_back(const_cast<char *>(command.c_str()));
		argv.push_back(const_cast<char *>("-f"));
		argv.push_back(const_cast<char *>(path.c_str()));
	}
	else if (command.find("python") != std::string::npos)
	{
		argv.push_back(const_cast<char *>(command.c_str()));
		argv.push_back(const_cast<char *>("-W"));
		argv.push_back(const_cast<char *>("ignore"));
		argv.push_back(const_cast<char *>(path.c_str()));
	}
	else
		throw httpResponse(INTERNAL_SERVER_ERROR);
	argv.push_back(NULL);
	return argv;
}

std::vector<std::string> request::build_env()
{
	std::vector<std::string> env_str;
	env_str.push_back("REQUEST_METHOD=" + _method);
	env_str.push_back("SCRIPT_FILENAME=" + _path);
	env_str.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env_str.push_back("SERVER_PROTOCOL=HTTP/1.1");
	if (_method == "GET")
		env_str.push_back("QUERY_STRING=" + _query);
	env_str.push_back("REDIRECT_STATUS=200");
	if (_method == "POST")
	{
		std::ostringstream ss;
		ss << _body.size();
		env_str.push_back("CONTENT_LENGTH=" + ss.str());
		env_str.push_back("SCRIPT_NAME=" + _path);
		env_str.push_back("PATH_INFO=" + _path);
		env_str.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
	}
	else
		env_str.push_back("CONTENT_LENGTH=0");
	env_str.push_back("PATH=/usr/bin:/bin");
	return (env_str);
}

void request::execChild(const std::string &command, int outPipe[2], int inPipe[2])
{
	close(outPipe[0]);
	dup2(outPipe[1], STDOUT_FILENO);
	close(outPipe[1]);

	if (_method == "POST")
	{
		close(inPipe[1]);
		dup2(inPipe[0], STDIN_FILENO);
		close(inPipe[0]);
	}

	std::vector<char *> argv = buildArgv(command, _path);
	std::vector<std::string> env_str = build_env();
	std::vector<char *> envp;
	for (size_t i = 0; i < env_str.size(); ++i)
		envp.push_back(const_cast<char *>(env_str[i].c_str()));
	envp.push_back(NULL);

	// Chdir so relative pahts work
	std::string dir = _path.substr(0, _path.find_last_of('/'));
	chdir(dir.c_str());

	execve(argv[0], argv.data(), envp.data());
	std::cerr << "execve failed" << std::endl;
	exit(1);
}

/* Reads from child and makes shure it does not hang in case of an infinite loop */
std::string readChild(pid_t child, int outPipe[2])
{
	const int		CGI_TIMEOUT = 3; // seconds
	const int		CHECK_INTERVAL = 1; // seconds
	time_t			start = time(NULL);
	char			buf[BUFFER];
	std::string		response;
	fd_set 			readfds;
	struct timeval	tv;

	while (true)
	{
		// Check timeout before waiting
		if (difftime(time(NULL), start) > CGI_TIMEOUT)
		{
			kill(child, SIGKILL);
			waitpid(child, NULL, 0);
			throw httpResponse(GATEWAY_TIMEOUT);
		}

		FD_ZERO(&readfds);
		FD_SET(outPipe[0], &readfds);
		tv.tv_sec = CHECK_INTERVAL;
		tv.tv_usec = 0;

		int ready = select(outPipe[0] + 1, &readfds, NULL, NULL, &tv);
		if (ready < 0)
		{
			if (errno == EINTR)
				continue; // signal interruption, just retry
			throw httpResponse(INTERNAL_SERVER_ERROR);

		}
		if (ready == 0)
			continue; // nothing to read yet, recheck timeout

		ssize_t n = read(outPipe[0], buf, sizeof(buf));
		if (n == 0)
			break; // EOF, child done
		if (n < 0)
		{
			if (errno != EINTR)
				throw httpResponse(INTERNAL_SERVER_ERROR);
			continue;
		}

		response.append(buf, n);
	}
	close(outPipe[0]);
	return response;
}

void request::handleParent(pid_t child, int outPipe[2], int inPipe[2])
{
	close(outPipe[1]);
	if (_method == "POST")
	{
		close(inPipe[0]);
		write(inPipe[1], _body.c_str(), _body.size());
		close(inPipe[1]);
	}
	std::string response = readChild(child, outPipe);

	int status = 0;
	waitpid(child, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		throw httpResponse(INTERNAL_SERVER_ERROR);
	// Split headers and body
	size_t headerEnd = response.find("\r\n\r\n");
	std::string cgiHeaders;
	std::string cgiBody;

	if (headerEnd != std::string::npos)
	{
		cgiHeaders = response.substr(0, headerEnd);
		cgiBody = response.substr(headerEnd + 4);
	}
	else
		cgiBody = response;
	size_t pos = cgiHeaders.find("content-type:");
	if (pos != std::string::npos)
	{
		size_t end = cgiHeaders.find("\r\n", pos);
		_contentType = cgiHeaders.substr(pos + 13, end - pos - 13);
	}
	else
		_contentType = "text/html";
	_body = cgiBody;
}

void request::cgi(std::string command)
{
	if (command == "")
		throw httpResponse(FORBIDEN);
	int outPipe[2];
	if (pipe(outPipe) == -1)
		throw httpResponse(INTERNAL_SERVER_ERROR);

	int inPipe[2];
	if (_method == "POST" && pipe(inPipe) == -1)
	{
		close(outPipe[0]);
		close(outPipe[1]);
		std::cerr << "CGI post inpipe failed" << std::endl;
		throw httpResponse(INTERNAL_SERVER_ERROR);
	}

	pid_t child = fork();
	if (child == -1)
	{
		close(outPipe[0]);
		close(outPipe[1]);
		if (_method == "POST")
		{
			close(inPipe[0]);
			close(inPipe[1]);
		}
		std::cerr << "CGI fork failed" << std::endl;
		throw httpResponse(INTERNAL_SERVER_ERROR);
	}
	_path = getAbsolutePath(_path);
	if (child == 0)
		execChild(command, outPipe, inPipe);
	else
		handleParent(child, outPipe, inPipe);
}