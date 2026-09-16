#include <iostream>
#include "includes/server.hpp"

int main()
{
	std::cout << "TEST 1 IRC" << std::endl;

	Server serv(4998);
	while (1)
		sleep(1);
}