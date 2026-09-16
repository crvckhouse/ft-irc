#include "../includes/server.hpp"

Server::Server(int port)
{
	_port = port;
	_serverFd = createSocket();
	if (_serverFd == -1)
		throw std::runtime_error("socket() failed");
	bindSocket();
}

Server::~Server()
{
	close(_serverFd);
}

int Server::createSocket()
{
	int fd;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	std::cout << "Socket id : " << fd << std::endl;
	if (fd == -1)
		return (-1);
	return (fd);
}
void Server::bindSocket()
{
	struct sockaddr_in serverAddress;

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(_port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	if (bind(_serverFd,(struct sockaddr *)&serverAddress,sizeof(serverAddress)) == -1)
		throw std::runtime_error("socket bend Error");
	std::cout << "Socket bended at port : " << _port << std::endl;

}