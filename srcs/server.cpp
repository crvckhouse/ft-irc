#include "../includes/server.hpp"

Server::Server(int port)
{
	_port = port;
	_serverFd = createSocket();
	if (_serverFd == -1)
		throw std::runtime_error("socket() failed");
	setSocketOptions();
	bindSocket();
	Listener();
	_clientFD = clientAccept();
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

	if (bind(_serverFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
		throw std::runtime_error("socket bend Error");
	std::cout << "Socket bended at port : " << _port << std::endl;
}

void Server::Listener()
{
	if (listen(_serverFd, 10) == -1)
		throw std::runtime_error("Listen() failed");
	std::cout << "Server is listening at port :" << _port << std::endl;
}
int Server::clientAccept()
{
	int clientFd = accept(_serverFd, NULL, NULL);
	if (clientFd == -1)
	{
		std::cout << "Accept() error" << std::endl;
		return (-1);
	}
	std::cout << "Client connected!" << std::endl;
	return clientFd;
}
void Server::setSocketOptions()
{
	int opt;

	opt = 1;

	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR,
				   &opt, sizeof(opt)) == -1)
		throw std::runtime_error("setsockopt() failed");
}

void Server::setupPoll()
{
	struct pollfd serverPollFd;

	serverPollFd.fd = _serverFd;
	serverPollFd.events = POLLIN;
	serverPollFd.revents = 0;

	_pollFds.push_back(serverPollFd);
}