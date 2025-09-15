#pragma once

#include "WebServer.hpp"

class setConf: public conf
{
	private:
		setConf();

	public:
		static void	parseFile(std::string filename);
		static void	setServer();
		static void	setEpoll();
};
