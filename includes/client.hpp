#pragma once
#include <string>
struct client
{
	int clientFD;
	bool auth;
	bool nick;
	bool user;
	bool registered;
	std::string buffer;
	std::string userName;
	std::string nickName;
};
