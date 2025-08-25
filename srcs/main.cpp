#include "request.hpp"
#include <cstdlib>
#include <sys/socket.h>
#include <cstdio>

#define PORT 3030

int main()
{
	int server = socket(AF_INET, SOCK_STREAM, 0);
	int flags = fcntl(server, F_GETFL, 0);
	if (flags == -1) perror("fcntl get");
	if (fcntl(server, F_SETFL, flags | O_NONBLOCK) == -1) perror("fcntl set");

	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT);
	
	int opt = 1;
	if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(1);
	}

	if (bind(server, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(1);
	}
	if (listen(server, SOMAXCONN) < 0) { // SOMAXCONN is max queue backlog
		perror("listen");
		exit(1);
	}
	
	request req(server);

	while (1)
	{
		try
		{
			req.listen();
		}
		catch(const std::exception& e)
		{
			req.~request();
			std::cerr << e.what() << '\n';
		}
	}
	close(server);
}
