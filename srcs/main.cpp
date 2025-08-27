#include "WebServer.hpp"

#define PORT 3030

int main()
{
//	if (argc != 2)
//		return (0);
//	try {
//		conf::setConfig(argv[2]);
//	}
	
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

//	while (1)
//	{
//		vector response	res;

		try
		{
			req.listen();
//			res = makeResponse(req);
		}
		catch(const std::exception& e)
		{
//			res = makeErrorResponse(e);
		}

//		res.sendResponse();
//	}
	close(server);
}
