#include "WebServer.hpp"

bool request::isCgiScript(std::string filename)
{
	size_t pos = filename.find_last_of(".");

	if (pos == std::string::npos)
		return 0;
	std::string extension = filename.substr(pos + 1);
	std::map<std::string, std::string> ext = _location.getCgiExtensions();
	std::map<std::string, std::string>::iterator it = ext.find(extension);
	if (it == ext.end())
		_cgiCommand = "";
	else
		_cgiCommand = it->second;
	return (_cgiCommand != "");
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
	argv.push_back(const_cast<char *>(command.c_str()));
	if (command.find("php") != std::string::npos)
		argv.push_back(const_cast<char *>("-f"));
	else if (command.find("python") != std::string::npos)
	{
		argv.push_back(const_cast<char *>("-W"));
		argv.push_back(const_cast<char *>("ignore"));
	}
	argv.push_back(const_cast<char *>(path.c_str()));
	argv.push_back(NULL);
	return argv;
}

std::vector<std::string> request::build_env(std::string path)
{
	std::vector<std::string> env_str;
	env_str.push_back("REQUEST_METHOD=" + _method);
	env_str.push_back("SCRIPT_FILENAME=" + _path);
	env_str.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env_str.push_back("SERVER_PROTOCOL=" + _protocol);
	env_str.push_back("REDIRECT_STATUS=200");
	if (_method == "GET")
		env_str.push_back("QUERY_STRING=" + _query);
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
	env_str.push_back("PATH=" + path);
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
	
	// Chdir so relative pahts work
	std::string dir = _path.substr(0, _path.find_last_of('/'));
	chdir(dir.c_str());

	std::vector<char *> argv = buildArgv(_cgiCommand, _path);
	std::vector<std::string> env_str = build_env(dir);
	std::vector<char *> envp;
	for (size_t i = 0; i < env_str.size(); ++i)
		envp.push_back(const_cast<char *>(env_str[i].c_str()));
	envp.push_back(NULL);


	execve(argv[0], argv.data(), envp.data());
	std::cerr << "execve failed: " << strerror(errno) << " (errno = " << errno << ")" << std::endl;
	_exit(127);
}

static StatusCode myRead(int fd, std::string &_buffer)
{
	char buf[BUFFER];

	ssize_t n = read(fd, buf, sizeof(buf));
	if (n > 0) {
		_buffer.append(buf, n);
		return REPEAT;         // more data may come
	}
	if (n < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
            return REPEAT;     // no data available yet
        if (errno == EINTR)
            return REPEAT;     // interrupted, retry
        return READ_ERROR; 
	}
	if (n == 0)
		return OK; // EOF: child is done writing
	return REPEAT;
}

static size_t findHeaderEnd(std::string buf)
{
	size_t pos1 = buf.find("\r\n\r\n"); // most common
	size_t pos2 = buf.find("\n\n");		// python
	size_t pos3 = buf.find("\r\r");		// extremely rare but valid

	// choose the earliest valid
	if (pos1 != std::string::npos)
		return pos1 + 4;
	else if (pos2 != std::string::npos)
		return pos2 + 2;
	else if (pos3 != std::string::npos)
		return pos3 + 2;
	return std::string::npos;
}

StatusCode request::cgi()
{
	StatusCode code = myRead(_infile, _responseBody);

	if (code == OK)
	{
		// child is DONE
		close(_infile);             // <-- prevent CGI from being called again
		_infile = -1;
		_cgiHeaderCheck = false;    // <-- reset state
		// do NOT reuse old _responseBody
		int status;
		if (waitpid(_cgiChild, &status, 0) == -1)
			return INTERNAL_SERVER_ERROR;

		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		{
			std::cout << "error 500" << std::endl;
			return INTERNAL_SERVER_ERROR;
		}

		return FINISHED;
	}
	if (code == READ_ERROR)
		return INTERNAL_SERVER_ERROR; 

	if (code == REPEAT)
	{
		if (_cgiHeaderCheck == false)
		{
			size_t end = findHeaderEnd(_responseBody);
			if (end == std::string::npos)
				return REPEAT;

			_responseBody.erase(0, end);
			_cgiHeaderCheck = true;
		}
		return REPEAT;
	}
	return INTERNAL_SERVER_ERROR;
}

bool request::cgiSetup()
{
	int outPipe[2];
	if (pipe(outPipe) == -1)
		return false;

	int inPipe[2];
	if (_method == "POST" && pipe(inPipe) == -1)
	{
		close(outPipe[0]);
		close(outPipe[1]);
		std::cerr << "CGI post inpipe failed" << std::endl;
		return false;
	}
	if (!setNonBlocking(outPipe[0]) || !setNonBlocking(outPipe[1]))
		return (close(_fd), false);
	
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
		return false;
	}
	if (!getAbsolutePath(_path)) return false;

	if (_cgiChild == 0) execChild(outPipe, inPipe);

	close(outPipe[1]);
	if (_method == "POST")
	{
		close(inPipe[0]);
		write(inPipe[1], _body.c_str(), _body.size());
		close(inPipe[1]);
	}
	_infile = outPipe[0];
	_cgiHeaderCheck = false;
	_responseBody.clear();
	return true;
}
