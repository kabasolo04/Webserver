#include "WebServer.hpp"

bool	isCgiScript(std::string filename)
{
	size_t pos = filename.find(".");
	if (pos == std::string::npos)
		throw httpResponse(BAD_REQUEST);
	std::string extension = filename.substr(pos + 1);
	if (extension != "php");
		return false;
	return true;
}


std::string	read_from_pipe(int fd) {
	char buffer[BUFFER + 1];
	std::string retval;
	ssize_t rd = read(fd, buffer, sizeof(buffer));

	if (rd == -1) {
		std::cerr << "Read failed!" << std::endl;
		return ("");
	}

	buffer[rd] = '\0';
	while (rd > 0) {
		retval += buffer;
		if (rd < 1024)
			break;
		rd = read(fd, buffer, 1024);
		buffer[rd] = '\0';
	}

	return retval;
}

void	cgi(File &responseBody, std::string path, std::string command)
{
	  int _pipe[2];
	if (pipe(_pipe) == -1) {
		std::cerr << "pipe failed" << std::endl;
		return;
	}

	pid_t child = fork();
	if (child == -1) {
		std::cerr << "fork failed" << std::endl;
		close(_pipe[0]);
		close(_pipe[1]);
		throw httpResponse(INTERNAL_SERVER_ERROR);
	}

	if (child == 0)
	{
		close(_pipe[0]); 
		dup2(_pipe[1], STDOUT_FILENO); 
		close(_pipe[1]);

		std::string file = path;
		if (file[0] == '/')
			file = file.substr(1);
		char *argv[] = {const_cast<char*>(command.c_str()), const_cast<char*>(file.c_str()), NULL};

		execve(argv[0], argv, conf::envp());
		
		std::cerr << "execve failed" << std::endl;
		exit(0);
	}
	else
	{
		int	childStatus;
		close(_pipe[1]); 
		waitpid(child, &childStatus, 0);
		if (childStatus != 0)
			throw httpResponse(INTERNAL_SERVER_ERROR);

		std::string response = read_from_pipe(_pipe[0]);
		responseBody.write(response.c_str());
		
		close(_pipe[0]); 
	}
}