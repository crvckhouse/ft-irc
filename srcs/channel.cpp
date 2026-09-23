#include "../includes/channel.hpp"
#include <iostream>
#include <string>

Channel::Channel(std::string channelName)
{
	_name = channelName;
}

Channel::~Channel()
{
}

bool Channel::hasMember()
{
	if (_members.size() == 0)
		return false;
	return true;
}

bool Channel::isMember(client *member)
{
	for (std::vector<client *>::iterator it = _members.begin(); it != _members.end(); ++it)
	{
		if (member == *it)
			return true;
	}
	return false;
}
void Channel::addMember(client *member)
{
	if (!isMember(member))
	{
		_members.push_back(member);
		std::cout << "MEMBER ADDED" << std::endl;
		return;
	}
	std::cout << "MEMBER ALREADY EXISTS" << std::endl;
}

void Channel::addOperator(client *member)
{
	_operators.push_back(member);
	std::cout << "OPERATOR ADDED" << std::endl;
}

void Channel::leaveChannel(client *member)
{
	removeOperator(member);
	for (std::vector<client *>::iterator it = _members.begin(); it != _members.end(); ++it)
	{
		if (*it == member)
		{
			std::cout << "DELETE MEMBERS" << std::endl;
			_members.erase(it);
		
			return;
		}
	}
}

void Channel::removeOperator(client *member)
{
	for (std::vector<client *>::iterator it = _operators.begin(); it != _operators.end(); ++it)
	{
		if (*it == member)
		{
			std::cout << "DELETE OPERATOR" << std::endl;
			_operators.erase(it);

			return;
		}
	}
}

std::string Channel::getName()
{
	return _name;
}