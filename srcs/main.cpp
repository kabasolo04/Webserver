#include "WebServer.hpp"

#define PORT 3030

int main()
{
//	if (argc != 2)
//		return (0);
//	try {
	conf::setConfig("Filename");
//	}
	

	request req(conf::server());

	while (1)
	{
//		vector response	res;

		try
		{
			req.listen();
//			res = makeResponse(req);
		}
		catch(const std::exception& e)
		{
//			res = makeErrorResponse(e);
			break;
		}

//		res.sendResponse();
	}
}
