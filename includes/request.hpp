#pragma once

#include "WebServer.hpp"

#define BUFFER 1

enum nodes
{
	READ_HEADER,
	READ_BODY,
	READ_CHUNKED,
	PROCESS,
	END,
	STATE_COUNT	// '\n' basically
};

class request
{
	private:
		serverConfig*	_server;

	protected:
		int									_fd;
		std::string							_method;
		std::string							_buffer;
		std::string							_path;
		std::string							_protocol;
		std::map<std::string, std::string>	_headers;
		std::string							_body;
		std::string							_contentType;
		std::string							_query;
		
		location*							_location;
		
		void (request::*_function)();

		bool	readUntil(std::string eof);
		//bool	readUntil(size_t size);

		//void		setReqLineVars();
		void		setHeaderVars();
		void		printHeaders();

		void			readRequestLine();
		void			readHeader();
		void			readBody();
		void			readChunked();
		virtual void	process();
		//void			response(StatusCode code);
		void			end();

		void						cgi(std::string command);
		std::string					isCgiScript(std::string filename);
		void						execChild(const std::string &command, int outPipe[2], int inPipe[2]);
		void						handleParent(pid_t child, int outPipe[2], int inPipe[2]);
		std::vector<std::string>	build_env();




		void	nextNode(nodes node);
	
		request(const request& other);
		request(int fd, std::string target, location* loc);
		request& operator = (const request& other);
		
	public:
		request(int fd, serverConfig& server);
		virtual ~request();

		const std::string& getContentType() const;
		const std::string& getBody() const;
		const std::string& getMethod() const;

		void	exec();
};
