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
	else
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
	argv.push_back(const_cast<char *>(command.c_str()));
	if (command.find("php-cgi") != std::string::npos)
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
	std::vector<std::string> env_str = build_env();
	std::vector<char *> envp;
	for (size_t i = 0; i < env_str.size(); ++i)
		envp.push_back(const_cast<char *>(env_str[i].c_str()));
	envp.push_back(NULL);

	// Chdir so relative pahts work
	std::string dir = _path.substr(0, _path.find_last_of('/'));
	chdir(dir.c_str());

	execve(argv[0], argv.data(), envp.data());
	std::cerr << "execve failed: " << strerror(errno) << " (errno = " << errno << ")" << std::endl;
	exit(1);
}

static void sendChunk(int fd, const std::string &data)
{
	std::ostringstream chunk;
	chunk << std::hex << data.size() << "\r\n";
	chunk << data << "\r\n";
	write(fd, chunk.str().c_str(), chunk.str().size());
}

static StatusCode	myRead(int fd, std::string& _buffer)
{
	char buffer[BUFFER];

	int len = read(fd, buffer, sizeof(buffer));

	if (len < 0)
		return READ_ERROR;

	if (len == 0)
		return CLIENT_DISCONECTED;

	_buffer.append(buffer, len);
 
	return REPEAT;
}

StatusCode request::cgi()
{
	StatusCode code = myRead(_infile, _buffer); // Koldo a metido el _buffer

	if (_cgiHeaderCheck == false)
	{
		//Strip the headers from the first reads
		size_t end = _buffer.find("\r\n\r\n");
		if (end == std::string::npos)
			return REPEAT;
		_buffer.erase(0, end + 4);

		//Tell browser that it is a chunked response
		std::ostringstream resp;
		resp << "HTTP/1.1 200 OK\r\n";
		resp << "Transfer-Encoding: chunked\r\n";
		resp << "Content-Type: " << _contentType << "\r\n";
		resp << "\r\n";
		write(_fd, resp.str().c_str(), resp.str().size());
		_cgiHeaderCheck = true;
	}

	if (code == REPEAT)
	{
		sendChunk(_fd, _buffer);
		_buffer.clear();
	}

	if (code != CLIENT_DISCONECTED)
		return REPEAT;

	write(_fd, "0\r\n\r\n", 5);
	// 2. IMPORTANT: Do NOT kill the child.
	// Just wait for it normally.
	int status = 0;
	waitpid(_cgiChild, &status, 0);

	// 3. If CGI crashed or returned non-zero → error
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		return INTERNAL_SERVER_ERROR;

	return FINISHED;	// Koldo a cabiao el return de EDN a FINISHED
}

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
	_cgiHeaderCheck = false;
	_buffer.clear();
	return FINISHED;
}
