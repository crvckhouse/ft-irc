#include "../includes/server.hpp"
#include "../includes/client.hpp"
#include "../includes/channel.hpp"
#include <cerrno>
#include <cstring>
#include <stdexcept>

Server::Server(const int port, const std::string passwd) : _port(port), _passwd(passwd)
{
	signal(SIGPIPE, SIG_IGN);
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
	if (fd == -1)
		return (-1);
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
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
	int flags = fcntl(clientFd, F_GETFL, 0);
	if (flags == -1 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1)
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
	while (true)
	{
		int pollRet;
		pollRet = poll(_pollFds.data(), _pollFds.size(), -1);
		if (pollRet == -1)
		{
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
						{
							disablePollout(fd);
						}
					}
					else if (bytesSent == -1 &&
							 errno != EINTR &&
							 errno != EAGAIN &&
							 errno != EWOULDBLOCK)
					{
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
					while (true)
					{
						client cl;
						_clientFD = clientAccept();

						if (_clientFD < 0)
							break;

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

void Server::sendNumber(int clientFD, int code,
						std::string nickname, std::string params, std::string text)
{
	std::ostringstream response;
	response << ":ft_irc ";

	if (code < 100)
		response << "0";
	if (code < 10)
		response << "0";

	response << code << " " << nickname;

	if (!params.empty())
		response << " " << params;

	if (!text.empty())
		response << " :" << text;

	response << "\r\n";
	sendMsg(clientFD, response.str());
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
client *Server::findClient(int fd)
{
	for (std::vector<client>::iterator it = _clients.begin();
		 it != _clients.end(); ++it)
	{
		if (it->clientFD == fd)
			return &(*it);
	}
	return NULL;
}

void Server::sendMsg(int clientFD, std::string msg)
{
	client *recipient = findClient(clientFD);
	if (recipient == NULL)
		return;
	if (!recipient->outputBuffer.empty())
	{
		recipient->outputBuffer += msg;
		enablePollout(clientFD);
		return;
	}

	ssize_t bytesSent = send(clientFD, msg.c_str(), msg.size(), MSG_NOSIGNAL);
	if (bytesSent == static_cast<ssize_t>(msg.size()))
		return;
	if (bytesSent == -1)
	{
		if (errno == EINTR)
		{
			recipient->outputBuffer += msg;
			enablePollout(clientFD);
			return;
		}
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			recipient->outputBuffer += msg;
			enablePollout(clientFD);
			return;
		}
		if (errno == EPIPE || errno == ECONNRESET)
		{
			std::cout << "Client connection closed while sending" << std::endl;
			return;
		}
		std::cout << "send() error: " << strerror(errno) << std::endl;
		return;
	}
	if (bytesSent > 0 &&
		bytesSent < static_cast<ssize_t>(msg.size()))
	{
		recipient->outputBuffer += msg.substr(bytesSent);
		enablePollout(clientFD);
		std::cout << "Partial send: " << bytesSent << " of "
				  << msg.size() << " bytes" << std::endl;
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

void Server::handleClose(client *client)
{
	client->buffer.clear();
	shutdown(client->clientFD, SHUT_RDWR);
}

void Server::handleNames(client *client, std::string channelName)
{
	if (channelName.empty())
	{
		sendNumber(client->clientFD, 366,
				   client->nickName, "*", "End of /NAMES list");
		return;
	}
	for (unsigned long i = 0; i < _Channels.size(); i++)
	{
		if (_Channels[i].getName() != channelName)
			continue;
		std::vector<int> members = _Channels[i].getMembers();
		std::string names;
		for (unsigned long j = 0; j < members.size(); j++)
		{
			for (unsigned long k = 0; k < _clients.size(); k++)
			{
				if (_clients[k].clientFD != members[j])
					continue;
				if (!names.empty())
					names += " ";
				if (_Channels[i].isOperator(members[j]))
					names += "@";
				names += _clients[k].nickName;
				break;
			}
		}
		sendNumber(client->clientFD, 353,
				   client->nickName, "= " + channelName, names);
		sendNumber(client->clientFD, 366,
				   client->nickName, channelName, "End of /NAMES list");
		return;
	}
	sendNumber(client->clientFD, 403,
			   client->nickName, channelName, "No such channel");
}

void Server::handleJoin(client *client, std::string channelName)
{
	std::string password;
	std::string channel_name;
	size_t pos = channelName.find(" ", 0);

	if (pos != std::string::npos)
	{
		size_t passPos = pos + 1;
		while (passPos < channelName.size() && channelName[passPos] == ' ')
			passPos++;
		password = channelName.substr(passPos);
		channelName = channelName.substr(0, pos);
	}
	for (std::vector<Channel>::iterator it = _Channels.begin(); it != _Channels.end(); ++it)
	{
		if (it->getName() == channelName && it->getInviteonly() == false)
		{
			if (it->getPass() != "" && (it->checkChannelPass(client->clientFD, password) == false))
			{
				sendNumber(client->clientFD, 475, client->nickName,
						   channelName, "Cannot join channel (+k)");
				return;
			}
			else
			{
				if (it->getMembersLimit() > 0 &&
					it->getMembersCount() >= it->getMembersLimit())
				{
					sendNumber(client->clientFD, 471, client->nickName,
							   channelName, "Cannot join channel (+l)");
					return;
				}
				else
				{
					if (it->isMember(client->clientFD))
					{
						sendNumber(client->clientFD, 443, client->nickName,
								   client->nickName + " " + channelName,
								   " is already on channel");
						return;
					}
					it->addMember(client->clientFD);
					it->addMembersCount();
					std::string response;
					response = ":";
					response += client->nickName;
					response += "!";
					response += client->nickName;
					response += "@localhost JOIN :";
					response += channelName;
					response += "\r\n";
					std::vector<int> members = it->getMembers();
					for (unsigned long k = 0; k < members.size(); k++)
						sendMsg(members[k], response);

					std::string names;
					for (unsigned long k = 0; k < members.size(); k++)
					{
						for (unsigned long l = 0; l < _clients.size(); l++)
						{
							if (_clients[l].clientFD != members[k])
								continue;
							if (!names.empty())
								names += " ";
							if (it->isOperator(members[k]))
								names += "@";
							names += _clients[l].nickName;
							break;
						}
					}
					sendNumber(client->clientFD, 353,
							   client->nickName, "= " + channelName, names);
					sendNumber(client->clientFD, 366,
							   client->nickName, channelName, "End of /NAMES list");
				}
			}
			return;
		}
		else if (it->getName() == channelName && it->getInviteonly() == true)
		{
			if (it->isInvited(client->clientFD) == false)
			{
				sendNumber(client->clientFD, 473, client->nickName,
						   channelName, "Cannot join channel (+i)");
				return;
			}
			if (it->getPass() != "" && (it->checkChannelPass(client->clientFD, password) == false))
			{
				sendNumber(client->clientFD, 475, client->nickName,
						   channelName, "Cannot join channel (+k)");
				return;
			}
			if (it->getMembersLimit() > 0 &&
				it->getMembersCount() >= it->getMembersLimit())
			{
				sendNumber(client->clientFD, 471, client->nickName,
						   channelName, "Cannot join channel (+l)");
				return;
			}
			if (it->isMember(client->clientFD))
			{
				sendNumber(client->clientFD, 443, client->nickName,
						   client->nickName + " " + channelName,
						   " is already on channel");
				return;
			}
			it->addMember(client->clientFD);
			it->addMembersCount();
			std::string response;
			response = ":";
			response += client->nickName;
			response += "!";
			response += client->nickName;
			response += "@localhost JOIN :";
			response += channelName;
			response += "\r\n";
			std::vector<int> members = it->getMembers();
			for (unsigned long k = 0; k < members.size(); k++)
				sendMsg(members[k], response);
			it->removeInvitation(client->clientFD);
			return;
		}
	}

	Channel newChannel(channelName, this);
	newChannel.addMember(client->clientFD);
	newChannel.addOperator(client->clientFD);
	_Channels.push_back(newChannel);
	std::string response;
	response = ":";
	response += client->nickName;
	response += "!";
	response += client->nickName;
	response += "@localhost JOIN :";
	response += channelName;
	response += "\r\n";
	sendMsg(client->clientFD, response);
	sendNumber(client->clientFD, 353,
			   client->nickName, "= " + channelName, "@" + client->nickName);
	sendNumber(client->clientFD, 366,
			   client->nickName, channelName, "End of /NAMES list");
}

void Server::handleNotice(client *sender, std::string argument)
{
	size_t pos = argument.find(" ", 0);
	std::cout << "NOTICE HANDLER CALLED" << std::endl;
	// std:: cout << "TESSSSST SEND2ALL" ;
	if (pos == std::string::npos)
		return;
	std::string dest = argument.substr(0, pos);
	while (pos < argument.size() && argument[pos] == ' ')
		pos++;
	std::string msg = argument.substr(pos, argument.size());
	if (msg.empty())
		return;
	if (msg[0] == ':')
		msg.erase(0, 1);

	std::cout << "ARGUMENT = [" << argument << "]" << std::endl;
	std::cout << "DEST = [" << dest << "]" << std::endl;
	std::cout << "MSG = [" << msg << "]" << std::endl;
	for (unsigned long i = 0; i < _clients.size(); i++)
	{
		if (_clients[i].nickName == dest)
		{
			std::string username = sender->userName;
			std::size_t usernameEnd = username.find(" ");

			if (usernameEnd != std::string::npos)
				username.erase(usernameEnd);

			std::string response;

			response = ":";
			response += sender->nickName;
			response += "!";
			response += username;
			response += "@localhost NOTICE ";
			response += dest;
			response += " :";
			response += msg;
			response += "\r\n";

			sendMsg(_clients[i].clientFD, response);
			return;
		}
	}
	for (unsigned long i = 0; i < _Channels.size(); i++)
	{
		if (_Channels[i].getName() == dest)
		{
			if (!(_Channels[i].isMember(sender->clientFD)))
			{
				return;
			}
			_Channels[i].sendNoticeToAll(sender, msg);
			return;
		}
	}
}

void Server::handlePart(client *client, std::string channelName)
{
	for (std::vector<Channel>::iterator it = _Channels.begin(); it != _Channels.end(); ++it)
	{
		if (it->getName() == channelName)
		{
			if (!it->isMember(client->clientFD))
			{
				sendNumber(client->clientFD, 442, client->nickName,
						   channelName, "You're not on that channel");
				return;
			}

			std::string username = client->userName;
			std::size_t usernameEnd = username.find(" ");
			if (usernameEnd != std::string::npos)
				username.erase(usernameEnd);

			std::string response = ":";
			response += client->nickName;
			response += "!";
			response += username;
			response += "@localhost PART ";
			response += channelName;
			response += "\r\n";

			std::vector<int> members = it->getMembers();
			for (unsigned long i = 0; i < members.size(); i++)
				sendMsg(members[i], response);

			it->leaveChannel(client);
			it->RemoveMembersCount();
			if (it->hasMember() == 0)
			{
				_Channels.erase(it);
				return;
			}
			if (it->hasOperator() == 0)
			{
				members = it->getMembers();
				if (!members.empty())
				{
					it->addOperator(members[0]);

					std::string modeResponse = ":ft_irc MODE ";
					modeResponse += channelName;
					modeResponse += " +o ";
					for (unsigned long i = 0; i < _clients.size(); i++)
					{
						if (_clients[i].clientFD == members[0])
						{
							modeResponse += _clients[i].nickName;
							break;
						}
					}
					modeResponse += "\r\n";

					std::vector<int> remaining = it->getMembers();
					for (unsigned long i = 0; i < remaining.size(); i++)
						sendMsg(remaining[i], modeResponse);
				}
			}
			return;
		}
	}
	std::cout << "CHANNEL NOT FOUND / CANNOT LEAVE" << std::endl;
}

void Server::handlePrivmsg(client *sender, std::string argument)
{

	size_t pos = argument.find(" ", 0);
	std::cout << "PRIVMSG HANDLER CALLED" << std::endl;
	// std:: cout << "TESSSSST SEND2ALL" ;
	if (pos == std::string::npos)
		return;
	std::string dest = argument.substr(0, pos);
	while (pos < argument.size() && argument[pos] == ' ')
		pos++;
	std::string msg = argument.substr(pos, argument.size());
	if (msg.empty())
	{
		sendNumber(sender->clientFD, 412, sender->nickName, "",
				   "No text to send");
		return;
	}
	if (msg[0] == ':')
		msg.erase(0, 1);

	std::cout << "ARGUMENT = [" << argument << "]" << std::endl;
	std::cout << "DEST = [" << dest << "]" << std::endl;
	std::cout << "MSG = [" << msg << "]" << std::endl;
	for (unsigned long i = 0; i < _clients.size(); i++)
	{
		if (_clients[i].nickName == dest)
		{
			std::string username = sender->userName;
			std::size_t usernameEnd = username.find(" ");

			if (usernameEnd != std::string::npos)
				username.erase(usernameEnd);

			std::string response = ":";
			response += sender->nickName;
			response += "!";
			response += username;
			response += "@localhost PRIVMSG ";
			response += dest;
			response += " :";
			response += msg;
			response += "\r\n";
			sendMsg(_clients[i].clientFD, response);
			return;
		}
	}
	for (unsigned long i = 0; i < _Channels.size(); i++)
	{
		if (_Channels[i].getName() == dest)
		{
			if (!(_Channels[i].isMember(sender->clientFD)))
			{
				sendNumber(sender->clientFD, 442, sender->nickName,
						   dest, "You're not on that channel");
				return;
			}
			_Channels[i].sendToAll(sender, msg);
			return;
		}
	}
	sendNumber(sender->clientFD, 401, sender->nickName, dest,
			   "No such nick/channel");
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
								std::string response = ":";
								response += client->nickName;
								response += "!";
								response += client->nickName;
								response += "@localhost KICK ";
								response += channel;
								response += " ";
								response += member;
								response += " :Kicked\r\n";
								std::vector<int> channelMembers =
									_Channels[i].getMembers();
								for (unsigned long l = 0;
									 l < channelMembers.size(); l++)
									sendMsg(channelMembers[l], response);
								_Channels[i].RemoveMembersCount();
								return;
							}
						}
					}
				}
				return;
			}
			sendNumber(client->clientFD, 482, client->nickName,
					   channel, "You're not channel operator");
		}
	}
}
void Server::handleInvite(client *client, std::string argument)
{
	size_t pos = argument.find(" ", 0);

	if (pos == std::string::npos)
		return;
	std::string invited = argument.substr(0, pos);
	while (argument[pos] == ' ')
		pos++;
	std::string channel = argument.substr(pos, argument.size());
	for (unsigned long i = 0; i < _Channels.size(); i++)
	{
		if (_Channels[i].getName() == channel)
		{
			if ((_Channels[i].isOperator(client->clientFD)))
			{

				for (unsigned long j = 0; j < _clients.size(); j++)
				{
					if (_clients[j].nickName == invited)
					{

						if (_Channels[i].getMembersCount() == _Channels[i].getMembersLimit())
						{
							sendNumber(client->clientFD, 471,
									   client->nickName, channel,
									   "Cannot invite user");
							return;
						}
						//_Channels[i].addMember(_clients[j].clientFD);
						//_Channels[i].addMembersCount();
						_Channels[i].addInvitation(_clients[j].clientFD);
						std::string response = ":";
						response += client->nickName;
						response += "!";
						response += client->nickName;
						response += "@localhost INVITE ";
						response += invited;
						response += " ";
						response += channel;
						response += "\r\n";
						sendMsg(_clients[j].clientFD, response);
						return;
					}
				}
				return;
			}
			return;
		}
	}
}
void Server::handleTopic(client *client, std::string argument)
{

	size_t pos = argument.find(" ", 0);
	std::string topic;
	std::string channel;
	if (pos == std::string::npos)
	{
		channel = argument;
		topic = "";
	}
	else
	{
		channel = argument.substr(0, pos);
		while (pos < argument.size() && argument[pos] == ' ')
			pos++;
		topic = argument.substr(pos);
	}
	if (!(topic.empty()) && topic[0] == ':')
		topic.erase(0, 1);
	for (unsigned long i = 0; i < _Channels.size(); i++)
	{
		if (_Channels[i].getName() == channel)
		{
			if (_Channels[i].isMember(client->clientFD))
			{
				if (topic.empty())
				{
					sendNumber(client->clientFD, 332, client->nickName,
							   channel, _Channels[i].getTopic());
				}
				else
				{
					if (_Channels[i].getTopicrestriction() == false ||
						(_Channels[i].getTopicrestriction() == true &&
						 _Channels[i].isOperator(client->clientFD) == true))
					{
						_Channels[i].newTopic(topic);
						std::string response = ":";
						response += client->nickName;
						response += "!";
						response += client->nickName;
						response += "@localhost TOPIC ";
						response += channel;
						response += " :";
						response += topic;
						response += "\r\n";
						std::vector<int> members =
							_Channels[i].getMembers();
						for (unsigned long j = 0; j < members.size(); j++)
							sendMsg(members[j], response);
					}
					else
						sendNumber(client->clientFD, 482, client->nickName,
								   channel, "You're not channel operator");
				}
			}
			else
				sendNumber(client->clientFD, 442, client->nickName,
						   channel, "You're not on that channel");
			return;
		}
	}
	sendNumber(client->clientFD, 403, client->nickName, channel,
			   "No such channel");
}

void Server::handleMode(client *client, std::string argument)
{
	std::string channel;
	std::string mode;
	std::string dest;
	size_t pos = 0;
	size_t temp_pos;

	pos = argument.find(" ");
	if (pos == std::string::npos)
	{
		sendNumber(client->clientFD, 461, client->nickName, "MODE",
				   "Not enough parameters");
		return;
	}
	channel = argument.substr(0, pos);

	pos++;
	while (pos < argument.size() && argument[pos] == ' ')
		pos++;
	temp_pos = pos;

	pos = argument.find(" ", temp_pos);
	if (pos == std::string::npos)
	{
		mode = argument.substr(temp_pos);
		dest = "";
	}
	else
	{
		mode = argument.substr(temp_pos, pos - temp_pos);
		pos++;
		while (pos < argument.size() && argument[pos] == ' ')
			pos++;
		dest = argument.substr(pos);
	}

	int targetFd = -1;
	if (mode == "+o" || mode == "-o")
	{
		if (dest.empty())
		{
			sendNumber(client->clientFD, 461, client->nickName,
					   "MODE", "Not enough parameters");
			return;
		}
		for (unsigned long j = 0; j < _clients.size(); j++)
		{
			if (_clients[j].nickName == dest)
			{
				targetFd = _clients[j].clientFD;
				break;
			}
		}
		if (targetFd == -1)
		{
			sendNumber(client->clientFD, 401, client->nickName,
					   dest, "No such nick");
			return;
		}
	}

	for (unsigned long i = 0; i < _Channels.size(); i++)
	{
		if (_Channels[i].getName() == channel)
		{
			_Channels[i].checkModeOp(client->clientFD, mode, dest, targetFd);
			return;
		}
		else
		{
			sendNumber(client->clientFD, 403, client->nickName,
					   channel, "No such channel");
		}
	}
}

void Server::handleQuit(client *client, std::string argument)
{
	std::string reason = argument;

	if (reason.empty())
		reason = "Quit";
	else if (reason[0] == ':')
		reason.erase(0, 1);

	std::string username = client->userName;
	std::size_t usernameEnd = username.find(" ");

	if (usernameEnd != std::string::npos)
		username.erase(usernameEnd);
	std::string response = ":";
	response += client->nickName;
	response += "!";
	response += username;
	response += "@localhost QUIT :";
	response += reason;
	response += "\r\n";

	for (unsigned int i = 0; i < _Channels.size();)
	{
		if (!_Channels[i].isMember(client->clientFD))
		{
			i++;
			continue;
		}
		std::vector<int> members = _Channels[i].getMembers();
		for (unsigned int j = 0; j < members.size(); j++)
		{
			if (members[j] != client->clientFD)
				sendMsg(members[j], response);
		}
		_Channels[i].leaveChannel(client);
		_Channels[i].RemoveMembersCount();
		if (!_Channels[i].hasMember())
		{
			_Channels.erase(_Channels.begin() + i);
			continue;
		}
		if (!_Channels[i].hasOperator())
		{
			std::vector<int> remaining = _Channels[i].getMembers();
			if (!remaining.empty())
			{
				_Channels[i].addOperator(remaining[0]);

				std::string modeResponse = ":ft_irc MODE ";
				modeResponse += _Channels[i].getName();
				modeResponse += " +o ";

				for (unsigned long j = 0; j < _clients.size(); j++)
				{
					if (_clients[j].clientFD == remaining[0])
					{
						modeResponse += _clients[j].nickName;
						break;
					}
				}
				modeResponse += "\r\n";

				for (unsigned long j = 0; j < remaining.size(); j++)
					sendMsg(remaining[j], modeResponse);
			}
		}
		i++;
	}
}

void Server::removeClient(int fd)
{
	client *clientToRemove = NULL;
	for (std::vector<client>::iterator it = _clients.begin();
		 it != _clients.end(); ++it)
	{
		if (it->clientFD == fd)
		{
			clientToRemove = &(*it);
			break;
		}
	}
	if (clientToRemove == NULL)
		return;

	handleQuit(clientToRemove, "");

	for (std::vector<Channel>::iterator it = _Channels.begin();
		 it != _Channels.end();)
	{
		it->removeInvitation(fd);
		++it;
	}

	close(fd);
	for (std::vector<struct pollfd>::iterator it = _pollFds.begin();
		 it != _pollFds.end(); ++it)
	{
		if (it->fd == fd)
		{
			_pollFds.erase(it);
			break;
		}
	}
	for (std::vector<client>::iterator it = _clients.begin();
		 it != _clients.end(); ++it)
	{
		if (it->clientFD == fd)
		{
			_clients.erase(it);
			break;
		}
	}
}
