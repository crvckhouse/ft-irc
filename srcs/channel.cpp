#include "../includes/channel.hpp"
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <cstdio>
#include <cstdlib>

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
	// send(fd, "ENTER CHANNEL PASSWORD:\n", 25, 0);
	if (passwd == _channelPassword)
	{
		send(fd, "GOOD PASS\n", 11, 0);
		return true;
	}
	else
	{
		send(fd, "BAD PASS\n", 10, 0);
		return false;
	}
}

void Channel::handlePass(int fd, std::string mode, std::string dest)
{
	if (mode == "-k")
	{
		_channelPassword = "";
		send(fd, "PASSWORD HAS BEEN REMOVED\n", 27, 0);
		return;
	}
	if (mode == "+k")
	{
		if (dest.size() == 0)
		{
			send(fd, "PLEASE ENTER PASSWORD\n", 23, 0);
			return;
		}
		_channelPassword = dest;

		send(fd, "PASSWORD CHANGED\n", 18, 0);
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
			send(fd, "CHANNEL HAS TOO MANY MEMBERS\n", 30, 0);
		else
		{
			_membersLimit = std::atoll(dest.c_str());
			send(fd, "MEMBERS LIMIT HAS BEEN SET TO ", 31, 0);
			send(fd, dest.c_str(), 40, 0);
			send(fd, "\n", 2, 0);
		}
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
	send(fd, "YOU ARE NOT CHANNEL OPERATOR\n", 30, 0);
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