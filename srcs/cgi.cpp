#include "WebServer.hpp"

char **g_envp = NULL;

bool	isCgiScript(std::string filename)
{
	size_t pos = filename.find_last_of(".");
	if (pos == std::string::npos) throw httpResponse(BAD_REQUEST);
	std::string extension = filename.substr(pos + 1);
	if (extension != "php")
		return false;
	return true;
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

	if (rd == -1) {
		std::cerr << "Read failed!" << std::endl;
		throw httpResponse(INTERNAL_SERVER_ERROR);
	}

	return retval;
}

void	cgi(std::string &responseBody, std::string path, std::string query, std::string command)
{
	int pipes[2];
	if (pipe(pipes) == -1)
	{
		std::cerr << "pipe failed" << std::endl;
		throw httpResponse(INTERNAL_SERVER_ERROR);
	}

	pid_t child = fork();
	if (child == -1)
	{
		std::cerr << "fork failed" << std::endl;
		close(pipes[0]);
		close(pipes[1]);
		throw httpResponse(INTERNAL_SERVER_ERROR);
	}

	if (child == 0)
	{
		close(pipes[0]); 
		dup2(pipes[1], STDOUT_FILENO); 
		close(pipes[1]);

		std::string file = path;

		std::vector<std::string> env;
		env.push_back("REQUEST_METHOD=GET");
		env.push_back("SCRIPT_FILENAME=" + path);
		env.push_back("QUERY_STRING=" + query);
		env.push_back("REDIRECT_STATUS=200");
		env.push_back("CONTENT_LENGTH=0");
		env.push_back("PATH=/usr/bin:/bin");

		// Build null-terminated envp[]
		std::vector<char*> envp;
		for (size_t i = 0; i < env.size(); i++)
			envp.push_back(const_cast<char*>(env[i].c_str()));
		envp.push_back(NULL);
		
		char *argv[3];
		argv[0] = const_cast<char*>(command.c_str());
		argv[1] = const_cast<char*>(file.c_str());
		argv[2] = NULL;

		//execve(argv[0], argv, conf::envp());
		execve(argv[0], argv, envp.data());
		std::cerr << "execve failed" << std::endl;
		exit(1);
	}
	else
	{
		close(pipes[1]); // parent closes write end

        std::string response;
        char buf[BUFFER];
        ssize_t n;
        while ((n = read(pipes[0], buf, sizeof(buf))) > 0)
            response.append(buf, n);
        close(pipes[0]);

        int status;
        waitpid(child, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
            throw httpResponse(INTERNAL_SERVER_ERROR);

        // Strip headers
        size_t headerEnd = response.find("\r\n\r\n");
        if (headerEnd != std::string::npos)
            responseBody.append(response.substr(headerEnd + 4));
        else
            responseBody.append(response);

	}
}