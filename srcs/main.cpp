#include "request.hpp"
#include <cstdlib>
#include <sys/socket.h>
#include <cstdio>

#define PORT 8080

int main()
{
	int server = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT);
	
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
