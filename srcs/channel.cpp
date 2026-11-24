#include "../includes/channel.hpp"
#include "../includes/server.hpp"
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <sstream>

void Channel::sendNumeric(int fd, int code, const std::string &params,
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
	_server->sendMsg(fd, response.str());
}

Channel::Channel(std::string channelName, Server *server)
{
	_server = server;
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

void Channel::addMember(int fd)
{
	if (!isMember(fd))
	{
		_members.push_back(fd);
		return;
	}
}

void Channel::addOperator(int fd)
{
	for (std::vector<int>::iterator it = _operators.begin(); it != _operators.end(); ++it)
	{
		if (*it == fd)
		{
			return;
		}
	}
	_operators.push_back(fd);
}

void Channel::leaveChannel(client *member)
{
	removeOperator(member->clientFD);
	for (std::vector<int>::iterator it = _members.begin(); it != _members.end(); ++it)
	{
		if (*it == member->clientFD)
		{
			_members.erase(it);
			return;
		}
	}
}

void Channel::removeOperator(int fd)
{
	for (std::vector<int>::iterator it = _operators.begin(); it != _operators.end(); ++it)
	{
		if (*it == fd)
		{
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
			sendNumeric(fd, 461, _name, "Not enough parameters");
			return;
		}
		_channelPassword = dest;
		return;
	}
}

void Channel::handleOperator(int fd, std::string mode, int targetFd)
{
	for (std::vector<int>::iterator it = _operators.begin(); it != _operators.end(); ++it)
	{
		if (*it == fd)
		{
			if (mode == "+o")
				addOperator(targetFd);
			else if (mode == "-o")
				removeOperator(targetFd);
			return;
		}
	}
}


void Channel::handleModeArg(int fd, std::string mode, std::string dest, int targetFd)
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
		handleOperator(fd, mode, targetFd);
	else if (mode == "+l" || mode == "-l")
	{
		handleLimit(fd, mode, dest);
	}
}

void Channel::sendModeMessage(int fd, const std::string &mode,
							  const std::string &dest)
{
	std::string nickname;
	for (std::vector<client>::iterator it = _server->_clients.begin();
		 it != _server->_clients.end(); ++it)
	{
		if (it->clientFD == fd)
		{
			nickname = it->nickName;
			break;
		}
	}
	if (nickname.empty())
		return;

	std::string message = ":";
	message += nickname + "!" + nickname + "@localhost MODE ";
	message += _name + " " + mode;
	if (!dest.empty())
		message += " " + dest;
	message += "\r\n";

	for (std::vector<int>::iterator it = _members.begin();
		 it != _members.end(); ++it)
		_server->sendMsg(*it, message);
}

void Channel::handleLimit(int fd, std::string mode, std::string dest)
{
	if (mode == "+l")
	{
		if (dest.empty())
		{
			sendNumeric(fd, 461, _name, "Not enough parameters");
			return;
		}
		for (std::size_t i = 0; i < dest.size(); i++)
		{
			if (!std::isdigit(static_cast<unsigned char>(dest[i])))
			{
				sendNumeric(fd, 461, _name, "Invalid channel limit");
				return;
			}
		}
		long long int limit = std::atoll(dest.c_str());
		if (limit <= 0 || _membersCount > limit)
			sendNumeric(fd, 461, _name, "Invalid channel limit");
		else
			_membersLimit = limit;
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
}
void Channel::RemoveMembersCount()
{
	_membersCount--;
}
long long int Channel::getMembersLimit()
{
	return _membersLimit;
}

void Channel::checkModeOp(int fd, std::string mode, std::string dest, int targetFd)
{
	for (std::vector<int>::iterator it = _operators.begin(); it != _operators.end(); ++it)
	{
		if (*it == fd)
		{
			bool oldInviteOnly = _inviteOnly;
			bool oldTopicRestriction = _topicRestriction;
			std::string oldPassword = _channelPassword;
			long long int oldLimit = _membersLimit;
			bool oldTargetOperator = isOperator(targetFd);
			handleModeArg(fd, mode, dest, targetFd);
			bool changed = oldInviteOnly != _inviteOnly ||
				oldTopicRestriction != _topicRestriction ||
				oldPassword != _channelPassword ||
				oldLimit != _membersLimit ||
				oldTargetOperator != isOperator(targetFd);
			if (changed)
				sendModeMessage(fd, mode, dest);
			return;
		}
	}
	sendNumeric(fd, 482, _name,
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
	for (unsigned long i = 0; i < _members.size(); i++)
	{
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
			_server->sendMsg(_members[i], response);
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

		_server->sendMsg(_members[i], response);
	}
}

void Channel::addInvitation(int fd)
{
	for (std::vector<int>::iterator it = _invited.begin(); it != _invited.end(); ++it) 
	{
		if (*it == fd)
			return;
	}
	_invited.push_back(fd);
}

bool Channel::isInvited(int fd)
{
	for (std::vector<int>::iterator it = _invited.begin(); it != _invited.end(); ++it) 
	{
		if (*it == fd)
			return true;
	}
	return false;
}

void Channel::removeInvitation(int fd)
{
	for (std::vector<int>::iterator it = _invited.begin(); it != _invited.end(); ++it) 
	{
		if (*it == fd)
		{
			_invited.erase(it);
			return;
		}
	}
}
