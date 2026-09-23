#pragma once
#include <string>
struct client
{
	int clientFD;
	bool auth;
	std::string buffer;
	std::string userName;
	std::string nickName;
};
