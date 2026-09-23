#pragma once
#include <string>
#include <vector>
#include "client.hpp"
class Channel
{
private:
	std::string _name;
	std::vector<client *> _members;
	std::vector<client *> _operators;
public:
	Channel(std::string channelName);
	~Channel();
	void addMember(client* member);
	void addOperator(client* member);
	void leaveChannel(client *member);
	void removeOperator(client *member);
	bool isMember(client *member);
	std::string getName();
};


