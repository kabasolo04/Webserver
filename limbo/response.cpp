#include "response.hpp"

void			Response::parse(std::string full_request)
{
	std::stringstream	fake_fd(full_request);
	std::string		line;
	std::string		ss;
	int			i;

	std::getline(fake_fd, line, '\r');
	if (line.empty() == true)	
		return ;
	ss = "";
	i = 0;
	while (line[i] != ' ')
	{
		ss.append(1, line[i]);
		i++;
	}
	this->method = ss;
	ss = "";
	i = 0;
	while (line[i] != ' ')
	{
		ss.append(1, line[i]);
		i++;
	}
	this->path = ss;
}

Response::Response(std::string full_request)
{
	std::cout << "Generating a response" << std::endl;
	/* En teoria me llega todo el string de la request para parsearla en el mismo constructor del response */
	parse(full_request);
	/* El de arriba lo estoy usando como prototipo, al final acabare repartiendolo entre los de abajo */
	/* parse_method(); */
	/* parse_status(); */
	/* parse_exists(); */
	/* parse_allowed(); */
	std::cout << "Method: " << this->method << "Path: " << this->path << std::endl;
}


Response::~Response()
{
	std::cout << "End of response" << std::endl;
}

const std::string	Response::send_response_header(const int socket_fd)
{
	/* Generar y enviar el header de la respuesta */
	(void)socket_fd;
	return (""+"\r\n\r\n\r\n\r\n");
}

const std::string	Response::send_response_body(const int socket_fd)
{
	/* Generar y enviar el body de la respuesta */
	(void)socket_fd;
	return ("");
}

