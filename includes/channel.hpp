#pragma once
#include <string>
#include <vector>
#include "client.hpp"
class Channel
{
private:
	std::string _name;
	// std::vector<client *> _members;
	std::vector<int> _operators;
	std::vector<int> _members;
	std::string _topic;
public:
	Channel(std::string channelName);
	~Channel();
	void addMember(int fd);
	void addOperator(int fd);
	void leaveChannel(client *member);
	void removeOperator(client *member);
	bool hasMember();
	bool hasOperator();
	bool isOperator(int fd);
	bool isMember(int fd);
	void newTopic(std::string topic);
	std::string getName();
	std::string getTopic();
	std::vector<int> getMembers();
	void  sendToAll(client *sender, std::string msg);
};


