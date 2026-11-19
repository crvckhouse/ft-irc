#include "../includes/server.hpp"
#include "../includes/client.hpp"
#include "../includes/channel.hpp"
#include <cerrno>
#include <cstring>
#include <stdexcept>

volatile sig_atomic_t Server::_running = 1;

void Server::handleSignal(int signal)
{
	(void)signal;
	Server::_running = 0;
}

Server::Server(const int port, const std::string passwd) : _port(port), _passwd(passwd)
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, Server::handleSignal);
	signal(SIGTERM, Server::handleSignal);
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
	cleanup();
}

void Server::cleanup()
{
	for (std::vector<client>::iterator it = _clients.begin();
		 it != _clients.end(); ++it)
		close(it->clientFD);
	_clients.clear();
	_pollFds.clear();
	if (_serverFd != -1)
	{
		close(_serverFd);
		_serverFd = -1;
	}
}
int Server::createSocket()
{
	int fd;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		return (-1);
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
	{
		close(fd);
		return (-1);
	}
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
}

void Server::Listener()
{
	if (listen(_serverFd, 10) == -1)
		throw std::runtime_error("Listen() failed");
}
int Server::clientAccept()
{
	int clientFd = accept(_serverFd, NULL, NULL);
	if (clientFd == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
			std::cout << "Accept() error" << std::endl;
		return (-1);
	}
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
	{
		close(clientFd);
		return (-1);
	}
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
	while (_running)
	{
		int pollRet;
		pollRet = poll(_pollFds.data(), _pollFds.size(), -1);
		if (pollRet == -1)
		{
			if (!_running && errno == EINTR)
				break;
			std::cout << "poll() error" << std::endl;
			continue;
		}
		unsigned long pollFdSize = _pollFds.size();
		for (long unsigned int i = 0; i < pollFdSize; i++)
		{
			std::cout << "CHECK FD " << _pollFds[i].fd
					  << " REVENTS = " << _pollFds[i].revents << std::endl;
			if (i != 0 && (_pollFds[i].revents & (POLLHUP | POLLERR | POLLNVAL)))
			{
				int fd = _pollFds[i].fd;
				std::cout << "Client socket error or hangup!" << std::endl;
				removeClient(fd);
				pollFdSize--;
				i--;
				continue;
			}
			if (_pollFds[i].revents & POLLOUT)
			{
				int fd = _pollFds[i].fd;
				client *recipient = findClient(fd);
				if (recipient != NULL && !recipient->outputBuffer.empty())
				{
					ssize_t bytesSent = send(fd,
											 recipient->outputBuffer.c_str(),
											 recipient->outputBuffer.size(), MSG_NOSIGNAL);
					if (bytesSent > 0)
					{
						recipient->outputBuffer.erase(0, bytesSent);
						if (recipient->outputBuffer.empty())
							disablePollout(fd);
					}
					else if (bytesSent == -1)
					{
						if (errno == EINTR ||
							errno == EAGAIN ||
							errno == EWOULDBLOCK)
							continue;
						std::cout << "send() error on fd " << fd
								  << ": " << strerror(errno) << std::endl;
						removeClient(fd);
						pollFdSize--;
						i--;
						continue;
					}
					else if (bytesSent == 0)
					{
						std::cout << "Client closed connection on fd "
								  << fd << std::endl;
						removeClient(fd);
						pollFdSize--;
						i--;
						continue;
					}
				}
				else if (recipient != NULL)
					disablePollout(fd);
			}
			if (_pollFds[i].revents & POLLIN)
			{
				std::cout << "POLLIN FD = "
						  << _pollFds[i].fd << std::endl;
				if (i == 0)
				{
					client cl;
					_clientFD = clientAccept();

					if (_clientFD < 0)
						continue;

					cl.clientFD = _clientFD;
					cl.buffer = "";
					cl.outputBuffer = "";
					cl.auth = false;
					cl.nick = false;
					cl.user = false;
					cl.registered = false;
					_clients.push_back(cl);

					std::cout << "CLIENTS SIZE = " << _clients.size() << std::endl;
					struct pollfd clientPollFd;
					clientPollFd.fd = _clientFD;
					clientPollFd.events = POLLIN;
					clientPollFd.revents = 0;
					_pollFds.push_back(clientPollFd);
				}
				else
				{
					int bytes_read;
					char BUFFER[1024];
					bytes_read = recv(_pollFds[i].fd, BUFFER,
									  sizeof(BUFFER) - 1, 0);
					if (bytes_read == -1 && errno == EINTR)
						continue;
					if (bytes_read > 0)
					{
						int fd = _pollFds[i].fd;
						BUFFER[bytes_read] = '\0';
						for (unsigned long j = 0; j < _clients.size(); j++)
						{
							if (fd == _clients[j].clientFD)
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
									std::cout << "COMMAND RECEIVED: [" << cmd << "]" << std::endl;
									int currentFd = _clients[j].clientFD;
									parseCmd(cmd, currentFd);
									if (findClient(currentFd) == NULL)
										break;
								}
							}
						}
						if (findClient(fd) == NULL)
						{
							pollFdSize--;
							i--;
							continue;
						}
						// std::cout << "Data received by client!: " << _pollFds[i].fd << " --->" << BUFFER << std::endl;
						// send(_pollFds[i].fd, BUFFER, bytes_read, 0);
					}
					else if (bytes_read == 0)
					{
						int fd = _pollFds[i].fd;
						std::cout << "Client disconnected!" << std::endl;
						removeClient(fd);
						pollFdSize--;
						i--;
					}
					else if (bytes_read == -1 &&
							 (errno == EAGAIN || errno == EWOULDBLOCK))
					{
						continue;
					}
					else
					{
						int fd = _pollFds[i].fd;
						std::cout << "recv() error: "
								  << strerror(errno) << std::endl;
						removeClient(fd);
						pollFdSize--;
						i--;
					}
				}
			}
		}
	}
}

void Server::tryRegister(client *client)
{
	if (client->registered)
		return;
	if (!client->auth || !client->nick || !client->user)
		return;

	client->registered = true;

	sendNumber(client->clientFD, 001, client->nickName, "",
			   "Welcome to ft_irc");

	sendNumber(client->clientFD, 002, client->nickName, "",
			   "Your host is ft_irc");

	sendNumber(client->clientFD, 003, client->nickName, "",
			   "This server was created today");

	sendNumber(client->clientFD, 004, client->nickName,
			   "ft_irc 1.0 oi", "");
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
	if (part1 == "CAP")
	{
		if (part2 == "LS 302" || part2 == "LS")
			sendMsg(clientFD, ":ft_irc CAP * LS :\r\n");
		return;
	}
	if (part1 == "PING")
	{
		if (part2.empty())
			sendMsg(clientFD, "PONG\r\n");
		else
			sendMsg(clientFD, "PONG " + part2 + "\r\n");
		return;
	}
	for (unsigned long j = 0; j < _clients.size(); j++)
	{
		if (_clients[j].clientFD != clientFD)
			continue;

		if (part1 == "PASS")
		{
			if (_clients[j].auth)
			{
				return;
			}
			if (part2 == _passwd)
			{
				_clients[j].auth = true;
				tryRegister(&_clients[j]);
			}
			else
			{
				sendNumber(clientFD, 464, "*", "", "Password incorrect");
			}
			return;
		}

		if (part1 == "NICK" && _clients[j].nick == false && _clients[j].clientFD == clientFD)
		{
			if (part2 == "")
			{
				sendNumber(clientFD, 431, "*", "", "No nickname given");
				return;
			}
			else
			{
				for (unsigned long k = 0; k < _clients.size(); k++)
				{
					if (_clients[k].clientFD != clientFD && _clients[k].nickName == part2)
					{
						sendNumber(clientFD, 433, "*", _clients[k].nickName,
								   "Nickname is already in use");
						return;
					}
				}
				_clients[j].nickName = part2;
				_clients[j].nick = true;
				tryRegister(&_clients[j]);
				return;
			}
		}

		if (part1 == "USER" && _clients[j].user == false && _clients[j].clientFD == clientFD)
		{
			if (part2.empty())
			{
				sendNumber(clientFD, 461, "*", "USER",
						   "Not enough parameters");
				return;
			}
			_clients[j].userName = part2;
			_clients[j].user = true;
			tryRegister(&_clients[j]);
			return;
		}

		if (!_clients[j].registered)
		{
			sendNumber(clientFD, 451, "*", "", "You have not registered");
			return;
		}

		if (_clients[j].registered)
		{
			if (part1 == "JOIN")
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
			else if (part1 == "INVITE")
			{
				handleInvite(&_clients[j], part2);
			}
			else if (part1 == "TOPIC")
			{
				handleTopic(&_clients[j], part2);
			}
			else if (part1 == "MODE")
			{
				handleMode(&_clients[j], part2);
			}
			else if (part1 == "NOTICE")
			{
				handleNotice(&_clients[j], part2);
			}
			else if (part1 == "QUIT")
			{
				removeClient(clientFD);
				return;
			}
			else if (part1 == "NAMES")
			{
				handleNames(&_clients[j], part2);
			}
			else
			{
				sendNumber(clientFD, 421, _clients[j].nickName, part1,
						   "Unknown command");
			}
			return;
		}
	}
}

void Server::enablePollout(int fd)
{
	for (std::vector<struct pollfd>::iterator it = _pollFds.begin();
		 it != _pollFds.end(); ++it)
	{
		if (it->fd == fd)
		{
			it->events |= POLLOUT;
			return;
		}
	}
}

void Server::disablePollout(int fd)
{
	for (std::vector<struct pollfd>::iterator it = _pollFds.begin();
		 it != _pollFds.end(); ++it)
	{
		if (it->fd == fd)
		{
			it->events &= ~POLLOUT;
			return;
		}
	}
}
