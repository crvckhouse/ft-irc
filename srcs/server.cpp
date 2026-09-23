
#include "../includes/server.hpp"
#include "../includes/client.hpp"
#include "../includes/channel.hpp"

Server::Server(const int port, const std::string passwd) : _port(port), _passwd(passwd)
{
	// _port = port;
	// _passwd = passwd;
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
						cl.auth = false;
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
								while (pos != std::string::npos)
								{
									std::string cmd;
									cmd = _clients[j].buffer.substr(0, pos);
									std::cout << "COMMAND = [" << cmd << "]" << std::endl;
									_clients[j].buffer.erase(0, pos + 2);
									pos = _clients[j].buffer.find("\r\n");
									parseCmd(cmd, _clients[j].clientFD);
								}
							}
						}
						std::cout << "Data received by client!: " << _pollFds[i].fd << " --->" << BUFFER << std::endl;
						// send(_pollFds[i].fd, BUFFER, bytes_read, 0);
					}
					else if (bytes_read == 0)
					{
						int fd = _pollFds[i].fd;
						close(fd);
						std::cout << "Client disconnected!" << std::endl;
						for (unsigned long j = 0; j < _clients.size(); j++)
						{
							if (_clients[j].clientFD == fd)
							{
								_clients.erase(_clients.begin() + j);
								break;
							}
						}
						_pollFds.erase(_pollFds.begin() + i);
						i--;
					}
					else
						std::cout << "recv() error!" << std::endl;
				}
			}
		}
	}
}

void Server::parseCmd(std::string cmd, int clientFD)
{
	std::string part1;
	std::string part2;

	std::size_t pos = cmd.find(" ");
	if (pos == std::string::npos)
	{
		part1 = cmd;
		part2 = "";
	}
	else
	{
		part1 = cmd.substr(0, pos);
		part2 = cmd.substr(pos + 1);
	}

	for (unsigned long j = 0; j < _clients.size(); j++)
	{
		if (_clients[j].clientFD == clientFD && _clients[j].auth == false)
		{
			if (part1 == "PASS" && part2 == _passwd)
			{
				_clients[j].auth = true;
				std::cout << "GOOD PASS" << std::endl;
			}
			else
				std::cout << "BAD PASSWORD" << std::endl;
		}
		if (_clients[j].auth == true && _clients[j].clientFD == clientFD)
		{
			if (part1 == "NICK")
			{
				_clients[j].nickName = part2;
			}
			else if (part1 == "USER")
			{
				_clients[j].userName = part2;
			}
			else if (part1 == "JOIN")
			{
				handleJoin(&_clients[j], part2);
			}
			else if (part1 == "PART")
			{
				handlePart(&_clients[j], part2);
			}

			break;
		}
	}
	std::cout << "FD = " << clientFD << std::endl;
	std::cout << "COMMAND = " << part1 << std::endl;
	std::cout << "ARGUMENT = " << part2 << std::endl;
	std::cout << _clients[0].nickName << std::endl;
}

void Server::handleJoin(client *client, std::string channelName)
{

	for (std::vector<Channel>::iterator it = _Channels.begin(); it != _Channels.end(); ++it)
	{
		if (it->getName() == channelName)
		{
			std::cout << "ADD MEMBER" << std::endl;
			it->addMember(client);

			return;
		}
	}

	Channel newChannel(channelName);
	newChannel.addMember(client);
	newChannel.addOperator(client);
	_Channels.push_back(newChannel);
	std::cout << "NEW CHANNEL NAME : " << channelName << std::endl;
}
void Server::handlePart(client *client, std::string channelName)
{
	for (std::vector<Channel>::iterator it = _Channels.begin(); it != _Channels.end(); ++it)
	{
		if (it->getName() == channelName)
		{
			it->leaveChannel(client);
			if (it->hasMember() == 0)
			{
				_Channels.erase(it);
				std::cout << "CHANNEL ERASED" << std::endl;
			}

			return;
		}
	}
	std::cout << "CHANNEL NOT FOUND / CANNOT LEAVE" << std::endl;
}
