#include <iostream>
#include <cstdlib>
#include "includes/server.hpp"

int main(int argc, char **argv)
{
	std::cout << "TEST 1 IRC" << std::endl;
	if (argc != 3)
		return (1);
	char *end;
	int port = strtol(argv[1],&end,10);
	if (port < 1 || port > 65535)
	{
		std::cerr << "Port number must be between 1 and 65535" << std::endl;
		return (1);
	}
	std::string pass = argv[2];
	if (pass == "")
	{
		std::cerr << "Password can't be empty." << std::endl;
		return (1);
	}
	Server serv(port, pass);
	return (0);
}