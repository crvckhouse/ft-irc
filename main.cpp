#include <iostream>
#include <cstdlib>
#include "includes/server.hpp"

int main(int argc, char **argv)
{
	std::cout << "TEST 1 IRC" << std::endl;
	if (argc != 3)
		return (1);

	Server serv(std::atoi(argv[1]), argv[2]);
	// while (1)
	// 	sleep(1);
	return (0);
}