#include "../includes/server.hpp"
#include "../includes/client.hpp"

Server::Server(int port)
{
	_port = port;
	_serverFd = createSocket();
	if (_serverFd == -1)
		throw std::runtime_error("socket() failed");
	setSocketOptions();
	bindSocket();
	Listener();
	setupPoll();
	run();
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
	std::cout << "Client connected!:" << clientFd << std::endl;
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
void Server::run()
{
	while (true)
	{
		int pollRet;
		pollRet = poll(_pollFds.data(), _pollFds.size(), -1);
		if (pollRet == -1)
		{
			std::cout << "poll() error" << std::endl;
			continue;
		}
		std::cout << "poll() returned !" << std::endl;
		std::cout << "revents = " << _pollFds[0].revents << std::endl;
		unsigned long pollFdSize = _pollFds.size();
		for (long unsigned int i = 0; i < pollFdSize; i++)
		{
			if (_pollFds[i].revents & POLLIN)
			{
				if (i == 0)
				{
					client cl;
					std::cout << "POLLIN detected!" << std::endl;
					_clientFD = clientAccept();
					if (_clientFD < 0)
						std::cout << "Client accept Error" << std::endl;
					else
					{
						cl.clientFD = _clientFD;
						cl.buffer = "";
						_clients.push_back(cl);
						struct pollfd clientPollFd;
						clientPollFd.fd = _clientFD;
						clientPollFd.events = POLLIN;
						clientPollFd.revents = 0;
						_pollFds.push_back(clientPollFd);
					}
				}
				else
				{
					int bytes_read;
					char BUFFER[1024];
					bytes_read = recv(_pollFds[i].fd, BUFFER, sizeof(BUFFER) - 1, 0);
					if (bytes_read > 0)
					{
						BUFFER[bytes_read] = '\0';
						for (unsigned long j = 0; j < _clients.size(); j++)
						{
							if (_pollFds[i].fd == _clients[j].clientFD)
							{
								_clients[j].buffer += BUFFER;
								std::size_t pos = _clients[j].buffer.find("\r\n", 0);
								if (pos != std::string::npos)
								{
									std::string cmd;
									cmd = _clients[j].buffer.substr(0, pos);
									std::cout << "COMMAND = [" << cmd << "]" << std::endl;
									_clients[j].buffer.erase(0, pos + 2);
									break;
								}
							}
						}
						std::cout << "Data received by client!: " << _pollFds[i].fd << " --->" << BUFFER << std::endl;
						// send(_pollFds[i].fd, BUFFER, bytes_read, 0);
					}
					else if (bytes_read == 0)
					{
						std::cout << "Client disconnected!" << std::endl;
						close(_pollFds[i].fd);
						_pollFds.erase(_pollFds.begin() + i);
						for (unsigned long j = 0; j < _clients.size(); j++)
						{
							if (_clients[j].clientFD == _pollFds[i].fd)
							{
								_clients.erase(_clients.begin() + j);
								break;
							}
						}
						i--;
					}
					else
						std::cout << "recv() error!" << std::endl;
				}
			}
		}
	}
}