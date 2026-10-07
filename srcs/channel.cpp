#include "../includes/channel.hpp"
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <cstdio>

Channel::Channel(std::string channelName)
{
	_name = channelName;
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
	for (std::vector<int >::iterator it = _operators.begin(); it != _operators.end(); ++it)
	{
		if (*it == member->clientFD)
		{
			// std::cout << "DELETE OPERATOR" << std::endl;
			_operators.erase(it);

			return;
		}
	}
}

std::string Channel::getName()
{
	return _name;
}

std::vector<int> Channel::getMembers()
{
	return _members;
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

			response = ":";
			response += sender->nickName;
			response += " PRIVMSG ";
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
// 10.171.55.7