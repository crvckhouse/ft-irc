#pragma once
#include <string>
struct client
{
	int clientFD;
	std::string buffer;
	std::string userName;
	std::string nickName;
};
