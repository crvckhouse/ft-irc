#pragma once
#include <string>
#include <vector>
#include "client.hpp"
class Channel
{
private:
	std::string _name;
	// std::vector<client *> _members;
	std::vector<client *> _operators;
	std::vector<int> _members;
public:
	Channel(std::string channelName);
	~Channel();
	void addMember(client* member);
	void addOperator(client* member);
	void leaveChannel(client *member);
	void removeOperator(client *member);
	bool hasMember();
	bool isMember(int fd);
	std::string getName();
	void  sendToAll(client *sender, std::string msg);
};


