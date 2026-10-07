
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
	// std::cout << "Socket id : " << fd << std::endl;
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
	// std::cout << "Socket bended at port : " << _port << std::endl;
}

void Server::Listener()
{
	if (listen(_serverFd, 10) == -1)
		throw std::runtime_error("Listen() failed");
	// std::cout << "Server is listening at port :" << _port << std::endl;
}
int Server::clientAccept()
{
	int clientFd = accept(_serverFd, NULL, NULL);
	if (clientFd == -1)
	{
		std::cout << "Accept() error" << std::endl;
		return (-1);
	}
	// std::cout << "Client connected!:" << clientFd << std::endl;
	std::cout << "ACCEPTED CLIENT FD = " << clientFd << std::endl;
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
		// std::cout << "poll() returned !" << std::endl;
		// std::cout << "revents = " << _pollFds[0].revents << std::endl;
		unsigned long pollFdSize = _pollFds.size();
		for (long unsigned int i = 0; i < pollFdSize; i++)
		{
			std::cout << "CHECK FD " << _pollFds[i].fd
					  << " REVENTS = " << _pollFds[i].revents << std::endl;
			if (_pollFds[i].revents & POLLIN)
			{
				std::cout << "POLLIN FD = "
						  << _pollFds[i].fd << std::endl;
				if (i == 0)
				{
					client cl;
					// std::cout << "POLLIN detected!" << std::endl;
					_clientFD = clientAccept();

					if (_clientFD < 0)
						std::cout << "Client accept Error" << std::endl;
						else
						{
						cl.clientFD = _clientFD;
						cl.buffer = "";
						cl.auth = false;
						_clients.push_back(cl);

						std::cout << "CLIENTS SIZE = " << _clients.size() << std::endl;
						struct pollfd clientPollFd;
						clientPollFd.fd = _clientFD;
						clientPollFd.events = POLLIN;
						clientPollFd.revents = 0;
						_pollFds.push_back(clientPollFd);
						send(clientPollFd.fd,"                                            ~~~~~WELCOME TO FT-IRC~~~~~",72,0);
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
									// std::cout << "COMMAND = [" << cmd << "]" << std::endl;
									_clients[j].buffer.erase(0, pos + 2);
									pos = _clients[j].buffer.find("\r\n");
									parseCmd(cmd, _clients[j].clientFD);
								}
							}
						}
						// std::cout << "Data received by client!: " << _pollFds[i].fd << " --->" << BUFFER << std::endl;
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
						pollFdSize--;
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

	std::size_t pos = 0;
	while (pos < cmd.size() && cmd[pos] == ' ')
		pos++;
	cmd = cmd.substr(pos);

	pos = cmd.find(" ");
	if (pos == std::string::npos)
	{
		part1 = cmd;
		part2 = "";
	}
	else
	{
		part1 = cmd.substr(0, pos);

		while (pos < cmd.size() && cmd[pos] == ' ')
			pos++;
		part2 = cmd.substr(pos);
	}
	for (unsigned long j = 0; j < _clients.size(); j++)
	{
		if (_clients[j].clientFD == clientFD && _clients[j].auth == false)
		{
			if (_clients[j].auth == false)
			{
				if (part1 == "PASS" && part2 == _passwd)
				{
					_clients[j].auth = true;
					// std::cout << "GOOD PASS" << std::endl;
				}
				else
					std::cout << "BAD PASSWORD" << std::endl;

				break;
			}
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
			else if (part1 == "PRIVMSG")
			{
				handlePrivmsg(&_clients[j], part2);
			}
			else if (part1 == "KICK")
			{
				handleKick(&_clients[j], part2);
			}
			break;
		}
	}
	// std::cout << "FD = " << clientFD << std::endl;
	// std::cout << "COMMAND = " << part1 << std::endl;
	// std::cout << "ARGUMENT = " << part2 << std::endl;
	// std::cout << _clients[0].nickName << std::endl;
}

void Server::handleJoin(client *client, std::string channelName)
{

	for (std::vector<Channel>::iterator it = _Channels.begin(); it != _Channels.end(); ++it)
	{
		if (it->getName() == channelName)
		{
			// std::cout << "ADD MEMBER" << std::endl;
			it->addMember(client->clientFD);

			return;
		}
	}

	Channel newChannel(channelName);
	newChannel.addMember(client->clientFD);
	newChannel.addOperator(client->clientFD);
	_Channels.push_back(newChannel);
	// std::cout << "NEW CHANNEL NAME : " << channelName << std::endl;
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
				// std::cout << "CHANNEL ERASED" << std::endl;
			}
			if (it->hasOperator() == 0)
			{
				std::vector<int> members = it->getMembers();
				it->addOperator(members[0]);
				send(members[0], "TEST OPERATOR EST PARTI MTN C TOI",34,0);
			}

			return;
		}
	}
	std::cout << "CHANNEL NOT FOUND / CANNOT LEAVE" << std::endl;
}

void Server::handlePrivmsg(client *sender, std::string argument)
{
	size_t pos = argument.find(" ", 0);

	// std:: cout << "TESSSSST SEND2ALL" ;
	if (pos == std::string::npos)
		return;
	std::string dest = argument.substr(0, pos);
	while (argument[pos] == ' ')
		pos++;
	std::string msg = argument.substr(pos, argument.size());
	// std::cout << "MSG:" << msg << std::endl;
	// std::cout << "DEST:" << dest << std::endl;
	if (msg[0] == ':')
		msg.erase(0, 1);

	for (unsigned long i = 0; i < _clients.size(); i++)
	{
		if (_clients[i].nickName == dest)
		{
			send(_clients[i].clientFD, sender->nickName.c_str(), sender->nickName.size(), 0);
			send(_clients[i].clientFD, ": ", 2, 0);
			send(_clients[i].clientFD, msg.c_str(), msg.size(), 0);
			return;
		}
	}
	for (unsigned long i = 0; i < _Channels.size(); i++)
	{
		if (_Channels[i].getName() == dest)
		{
			if (!(_Channels[i].isMember(sender->clientFD)))
			{
				send(sender->clientFD, "YOU ARE NOT IN THIS CHANNEL", 28,0);
				return ;
			}
			_Channels[i].sendToAll(sender, msg);
			return;
		}
	}
	send(sender->clientFD, "THIS CHANNEL OR USER DOESNT EXIST", 34,0);

}

void Server::handleKick(client *client, std::string argument)
{
	size_t pos = argument.find(" ", 0);

	if (pos == std::string::npos)
		return;
	std::string channel = argument.substr(0, pos);
	while (argument[pos] == ' ')
		pos++;
	std::string member = argument.substr(pos, argument.size());
	for (unsigned long i = 0; i < _Channels.size(); i++)
	{
		if (_Channels[i].getName() == channel)
		{
			if ((_Channels[i].isOperator(client->clientFD)))
			{
				for (unsigned long j = 0; j < _clients.size(); j++)
				{
					if (_clients[j].nickName == member)
					{
						std::vector<int> members = _Channels[i].getMembers();
						for (unsigned long k = 0; k < members.size(); k++)
						{
							if (_clients[j].clientFD == members[k])
							{
								_Channels[i].leaveChannel(&_clients[j]);
								send(_clients[j].clientFD,"TU T FAIT BAN", 14,0);
								send(client->clientFD,"LA PERSONNE A ETE BAN", 22,0);
								return ;
							}
						}

					}
				}
				return ;
			}
			send(client->clientFD, "YOU ARE NOT CHANNEL OPERATOR", 29,0);
		}
	}
}
