#include "../includes/channel.hpp"
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <cstdio>
#include <cstdlib>
#include <sstream>

static void sendChannelNumeric(int fd, int code, const std::string &params,
							   const std::string &text)
{
	std::ostringstream response;
	response << ":ft_irc ";
	if (code < 100)
		response << "0";
	if (code < 10)
		response << "0";
	response << code << " *";
	if (!params.empty())
		response << " " << params;
	if (!text.empty())
		response << " :" << text;
	response << "\r\n";
	std::string message = response.str();
	send(fd, message.c_str(), message.size(), 0);
}

Channel::Channel(std::string channelName)
{
	_name = channelName;
	_inviteOnly = false;
	_topicRestriction = false;
	_channelPassword = "";
	_membersLimit = 0;
	_membersCount = 1;
}

Channel::~Channel()
{
}

bool Channel::hasOperator()
{
	if (_operators.size() == 0)
		return false;
	return true;
}

bool Channel::hasMember()
{
	if (_members.size() == 0)
		return false;
	return true;
}

bool Channel::isMember(int fd)
{
	for (long unsigned int i = 0; i < _members.size(); i++)
	{
		if (_members[i] == fd)
			return true;
	}
	return false;
}

bool Channel::isOperator(int fd)
{
	for (long unsigned int i = 0; i < _operators.size(); i++)
	{
		if (_operators[i] == fd)
			return true;
	}
	return false;
}
// void Channel::addMember(client *member)
// {
// 	if (!isMember(member->clientFD))
// 	{
// 		_members.push_back(member->clientFD);
// 		// std::cout << "MEMBER ADDED" << std::endl;
// 		return;
// 	}
// 	std::cout << "MEMBER ALREADY EXISTS" << std::endl;
// }

void Channel::addMember(int fd)
{
	std::cout << "ADDING FD " << fd
			  << " TO CHANNEL " << _name << std::endl;

	if (!isMember(fd))
	{
		_members.push_back(fd);

		std::cout << "MEMBERS NOW = " << _members.size() << std::endl;

		for (unsigned long i = 0; i < _members.size(); i++)
			std::cout << "MEMBER FD = " << _members[i] << std::endl;

		return;
	}

	std::cout << "MEMBER ALREADY EXISTS" << std::endl;
}

void Channel::addOperator(int fd)
{
	_operators.push_back(fd);
	// std::cout << "OPERATOR ADDED" << std::endl;
}

void Channel::leaveChannel(client *member)
{
	removeOperator(member);
	for (std::vector<int>::iterator it = _members.begin(); it != _members.end(); ++it)
	{
		if (*it == member->clientFD)
		{
			// std::cout << "DELETE MEMBERS" << std::endl;
			_members.erase(it);

			return;
		}
	}
}

void Channel::removeOperator(client *member)
{
	for (std::vector<int>::iterator it = _operators.begin(); it != _operators.end(); ++it)
	{
		if (*it == member->clientFD)
		{
			// std::cout << "DELETE OPERATOR" << std::endl;
			_operators.erase(it);

			return;
		}
	}
}
std::string Channel::getPass()
{
	return _channelPassword;
}

bool Channel::checkChannelPass(int fd, std::string passwd)
{
	(void)fd;
	return passwd == _channelPassword;
}

void Channel::handlePass(int fd, std::string mode, std::string dest)
{
	if (mode == "-k")
	{
		_channelPassword = "";
		return;
	}
	if (mode == "+k")
	{
		if (dest.size() == 0)
		{
			sendChannelNumeric(fd, 461, _name,
							   "Not enough parameters");
			return;
		}
		_channelPassword = dest;
		return;
	}
}

void Channel::handleOperator(int fd, std::string mode, std::string dest)
{
	for (std::vector<int>::iterator it = _operators.begin(); it != _operators.end(); ++it)
	{
		if (*it == fd)
		{
			handleModeArg(fd, mode, dest);
			return;
		}
	}
}

void Channel::handleModeArg(int fd, std::string mode, std::string dest)
{
	if (mode == "+i")
	{
		_inviteOnly = true;
		return;
	}
	if (mode == "-i")
	{
		_inviteOnly = false;
		return;
	}
	if (mode == "+t")
	{
		_topicRestriction = true;
		return;
	}
	if (mode == "-t")
	{
		_topicRestriction = false;
		return;
	}
	else if (mode == "+k" || mode == "-k")
	{
		handlePass(fd, mode, dest);
	}
	else if (mode == "+o" || mode == "-o")
		handleOperator(fd, mode, dest);
	else if (mode == "+l" || mode == "-l")
	{
		handleLimit(fd, mode, dest);
	}
}

void Channel::handleLimit(int fd, std::string mode, std::string dest)
{
	if (mode == "+l")
	{
		if (_membersCount > std::atoll(dest.c_str()))
			sendChannelNumeric(fd, 461, _name,
							   "Invalid channel limit");
		else
			_membersLimit = std::atoll(dest.c_str());
	}
	if (mode == "-l")
	{
		_membersLimit = 0;
	}
}
long long int Channel::getMembersCount()
{
	return _membersCount;
}
void Channel::addMembersCount()
{
	_membersCount++;
	std::cout << "MEMBERS COUNT :" << _membersCount << std::endl;
}
void Channel::RemoveMembersCount()
{
	_membersCount--;
}
long long int Channel::getMembersLimit()
{
	return _membersLimit;
}

void Channel::checkModeOp(int fd, std::string mode, std::string dest)
{
	for (std::vector<int>::iterator it = _operators.begin(); it != _operators.end(); ++it)
	{
		if (*it == fd)
		{
			handleModeArg(fd, mode, dest);
			return;
		}
	}
	sendChannelNumeric(fd, 482, _name,
					   "You're not channel operator");
}

std::string Channel::getName()
{
	return _name;
}

std::vector<int> Channel::getMembers()
{
	return _members;
}

void Channel::newTopic(std::string topic)
{
	_topic = topic;
}

std::string Channel::getTopic()
{
	return _topic;
}
bool Channel::getInviteonly()
{
	return _inviteOnly;
}

bool Channel::getTopicrestriction()
{
	return _topicRestriction;
}

void Channel::sendToAll(client *sender, std::string msg)
{
	std::cout << "===== SEND TO ALL =====" << std::endl;
	std::cout << "SENDER FD = " << sender->clientFD << std::endl;
	std::cout << "MEMBERS SIZE = " << _members.size() << std::endl;

	for (unsigned long i = 0; i < _members.size(); i++)
	{
		std::cout << "MEMBER[" << i << "] FD = "
				  << _members[i] << std::endl;

		if (_members[i] != sender->clientFD)
		{
			std::string response;
			std::string username = sender->userName;
			std::size_t usernameEnd = username.find(" ");
			if (usernameEnd != std::string::npos)
				username.erase(usernameEnd);
			response = ":";
			response += sender->nickName;
			response += "!";
			response += username;
			response += "@localhost PRIVMSG ";
			response += getName();
			response += " :";
			response += msg;
			response += "\r\n";

			std::cout << "SENDING TO FD " << _members[i] << std::endl;
			std::cout << "RESPONSE = [" << response << "]" << std::endl;

			int ret = send(_members[i], response.c_str(), response.size(), 0);

			std::cout << "SEND RETURN = " << ret << std::endl;

			if (ret == -1)
				perror("send");
		}
	}
}
void Channel::sendNoticeToAll(client *sender, std::string msg)
{
	std::string username = sender->userName;
	std::size_t usernameEnd = username.find(" ");

	if (usernameEnd != std::string::npos)
		username.erase(usernameEnd);

	for (unsigned long i = 0; i < _members.size(); i++)
	{
		if (_members[i] == sender->clientFD)
			continue;

		std::string response;

		response = ":";
		response += sender->nickName;
		response += "!";
		response += username;
		response += "@localhost NOTICE ";
		response += _name;
		response += " :";
		response += msg;
		response += "\r\n";

		send(_members[i], response.c_str(), response.size(), 0);
	}
}