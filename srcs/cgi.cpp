#include "WebServer.hpp"

std::string	isCgiScript(std::string filename)
{
	size_t pos = filename.find_last_of(".");
	if (pos == std::string::npos) throw httpResponse(BAD_REQUEST);
	std::string extension = filename.substr(pos + 1);
	if (extension == "php") return "/usr/bin/php-cgi";
	if (extension == "py") return "/usr/bin/python3";
	return "";
}

std::string	read_from_pipe(int fd) {
	char		buffer[BUFFER + 1];
	std::string	retval;
	ssize_t		rd;

	while ((rd = read(fd, buffer, BUFFER)) > 0)
	{
		buffer[rd] = '\0';
		retval.append(buffer, rd);
	}
	if (rd == -1)
	{
		std::cerr << "Read failed!" << std::endl;
		throw httpResponse(INTERNAL_SERVER_ERROR);
	}
	return retval;
}

std::vector<char*> buildArgv(const std::string& command, const std::string& path)
{
	std::vector<char*> argv;
	if (command.find("php-cgi") != std::string::npos)
	{
		argv.push_back(const_cast<char*>(command.c_str()));
		argv.push_back(const_cast<char*>("-f"));
		argv.push_back(const_cast<char*>(path.c_str()));
	}
	else if (command.find("python") != std::string::npos)
	{
		argv.push_back(const_cast<char*>(command.c_str()));
		argv.push_back(const_cast<char*>("-W"));
		argv.push_back(const_cast<char*>("ignore"));
		argv.push_back(const_cast<char*>(path.c_str()));
	}
	else
		throw httpResponse(INTERNAL_SERVER_ERROR);
	argv.push_back(NULL);
	return argv;
}

std::vector<std::string>	build_env(const request& req)
{
	std::vector<std::string> env_str;
	env_str.push_back("REQUEST_METHOD=" + req.getMethod());
	env_str.push_back("SCRIPT_FILENAME=" + req.getPath());
	env_str.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env_str.push_back("SERVER_PROTOCOL=HTTP/1.1");
	if (req.getMethod() == "GET")
		env_str.push_back("QUERY_STRING=" + req.getQuery());
	env_str.push_back("REDIRECT_STATUS=200");
	if (req.getMethod() == "POST")
	{
		std::ostringstream ss;
		ss << req.getBody().size();
		env_str.push_back("CONTENT_LENGTH=" + ss.str());
		env_str.push_back("SCRIPT_NAME=" + req.getPath());
		env_str.push_back("PATH_INFO=" + req.getPath());
		env_str.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
	}
	else
		env_str.push_back("CONTENT_LENGTH=0");
	env_str.push_back("PATH=/usr/bin:/bin");
	return (env_str);
}

void	execChild(const request& req, const std::string &command, int outPipe[2], int inPipe[2])
{
	close(outPipe[0]);
	dup2(outPipe[1], STDOUT_FILENO);
	close(outPipe[1]);

	if (req.getMethod() == "POST")
	{
		close(inPipe[1]);
		dup2(inPipe[0], STDIN_FILENO);
		close(inPipe[0]);
	}

	std::vector<char *> argv = buildArgv(command, req.getPath());
	std::vector<std::string> env_str = build_env(req);
	std::vector<char*> envp;
	for (size_t i = 0; i < env_str.size(); ++i)
		envp.push_back(const_cast<char*>(env_str[i].c_str()));
	envp.push_back(NULL);

	//Chdir so relative pahts work
	std::string dir = req.getPath().substr(0, req.getPath().find_last_of('/'));
	chdir(dir.c_str());

	execve(argv[0], argv.data(), envp.data());
	std::cerr << "execve failed" << std::endl;
	exit(1);
}

void handleParent(request& req, pid_t child, int outPipe[2], int inPipe[2])
{
	close(outPipe[1]);
	if (req.getMethod() == "POST")
	{
		close(inPipe[0]);
		write(inPipe[1], req.getBody().c_str(), req.getBody().size());
		close(inPipe[1]);
	}
	std::string response;
	char buf[BUFFER];
	ssize_t n;
	while ((n = read(outPipe[0], buf, sizeof(buf))) > 0)
		response.append(buf, n);
	close(outPipe[0]);

	int status;
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
		req.setContentType(cgiHeaders.substr(pos + 13, end - pos - 13));
	}
	else
		req.setContentType("text/html");
	req.setBody(cgiBody);
}

void	request::cgi(std::string command)
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
		execChild(*this, command, outPipe, inPipe);
	else
 		handleParent(*this, child, outPipe, inPipe);
}