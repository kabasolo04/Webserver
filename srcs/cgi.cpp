#include "WebServer.hpp"

StatusCode request::isCgiScript(std::string filename)
{
	size_t pos = filename.find_last_of(".");
	if (pos == std::string::npos)
		return BAD_REQUEST;
	std::string extension = filename.substr(pos + 1);
	std::map<std::string, std::string> ext = _location.getCgiExtensions();
	std::map<std::string, std::string>::iterator it = ext.find(extension);
    if (it == ext.end())
        _cgiCommand = "";
    _cgiCommand = it->second;
	return OK;
}

static bool	getAbsolutePath(std::string &path)
{
	char absPath[PATH_MAX];
	if (realpath(path.c_str(), absPath) == NULL)
		return false;
	path = std::string(absPath);
	return true;
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

void request::execChild(int outPipe[2], int inPipe[2])
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

	std::vector<char *> argv = buildArgv(_cgiCommand, _path);
	if (argv.empty()) exit(1);
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

StatusCode request::handleCgi()
{
	StatusCode code;
	code = readAndSend();
	if (code == REPEAT) return REPEAT;

	int status = 0;
	waitpid(_cgiChild, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		return INTERNAL_SERVER_ERROR;
	return code;
}

/* Reads from child and makes shure it does not hang in case of an infinite loop */
/* bool readChild(pid_t child, int outPipe[2], std::string *response)
{
	const int		CGI_TIMEOUT = 3; // seconds
	const int		CHECK_INTERVAL = 1; // seconds
	time_t			start = time(NULL);
	char			buf[BUFFER];
	fd_set 			readfds;
	struct timeval	tv;

	while (true)
	{
		// Check timeout before waiting
		if (difftime(time(NULL), start) > CGI_TIMEOUT)
		{
			kill(child, SIGKILL);
			waitpid(child, NULL, 0);
			return (GATEWAY_TIMEOUT);
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
			return (INTERNAL_SERVER_ERROR);

		}
		if (ready == 0)
			continue; // nothing to read yet, recheck timeout

		ssize_t n = read(outPipe[0], buf, sizeof(buf));
		if (n == 0)
			break; // EOF, child done
		if (n < 0)
		{
			if (errno != EINTR)
				return (INTERNAL_SERVER_ERROR);
			continue;
		}

		response->append(buf, n);
	}
	close(outPipe[0]);
	return OK;
} */

/* bool request::handleParent(pid_t child, int outPipe[2], int inPipe[2])
{
	close(outPipe[1]);
	if (_method == "POST")
	{
		close(inPipe[0]);
		write(inPipe[1], _body.c_str(), _body.size());
		close(inPipe[1]);
	}
	std::string response;
	if (readChild(child, outPipe, &response) == false) return 0;

	int status = 0;
	waitpid(child, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		return (0);
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
} */

StatusCode request::cgiSetup()
{
	int outPipe[2];
	if (pipe(outPipe) == -1)
		return (INTERNAL_SERVER_ERROR);

	int inPipe[2];
	if (_method == "POST" && pipe(inPipe) == -1)
	{
		close(outPipe[0]);
		close(outPipe[1]);
		std::cerr << "CGI post inpipe failed" << std::endl;
		return (INTERNAL_SERVER_ERROR);
	}

	_cgiChild = fork();
	if (_cgiChild == -1)
	{
		close(outPipe[0]);
		close(outPipe[1]);
		if (_method == "POST")
		{
			close(inPipe[0]);
			close(inPipe[1]);
		}
		std::cerr << "CGI fork failed" << std::endl;
		return (INTERNAL_SERVER_ERROR);
	}

	if (!getAbsolutePath(_path)) return INTERNAL_SERVER_ERROR;

	if (_cgiChild == 0) execChild(outPipe, inPipe);

	close(outPipe[1]);
	if (_method == "POST")
	{
		close(inPipe[0]);
		write(inPipe[1], _body.c_str(), _body.size());
		close(inPipe[1]);
	}
	_infile = outPipe[0];
	return OK;
}
