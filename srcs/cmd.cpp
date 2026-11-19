#include "../includes/server.hpp"
#include "../includes/client.hpp"
#include "../includes/channel.hpp"
#include <cerrno>
#include <cstring>
#include <stdexcept>

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