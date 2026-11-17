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
	bool _inviteOnly;
	bool _topicRestriction;
	std::string _channelPassword;
	long long int _membersLimit;
	long long int _membersCount;
	std::vector<int> _invited;
public:
	Channel(std::string channelName);
	~Channel();
	void addMember(int fd);
	void addOperator(int fd);
	void leaveChannel(client *member);
	void removeOperator(int fd);
	bool hasMember();
	bool hasOperator();
	bool isOperator(int fd);
	bool isMember(int fd);
	bool getInviteonly();
	bool getTopicrestriction();
	long long int getMembersCount();
	long long int getMembersLimit();
	void addMembersCount();
	void RemoveMembersCount();
	void newTopic(std::string topic);
	void handleModeArg(int fd, std::string mode, std::string dest, int targetFd);
	void checkModeOp(int fd, std::string mode, std::string dest, int targetFd);
	void handlePass(int fd, std::string mode, std::string dest);
	bool checkChannelPass(int fd, std::string passwd);
	void handleOperator(int fd, std::string mode, int targetFd);
	void handleLimit(int fd, std::string mode, std::string dest);
	std::string getPass();
	std::string getName();
	std::string getTopic();
	std::vector<int> getMembers();
	void  sendToAll(client *sender, std::string msg);
	void sendNoticeToAll(client *sender, std::string msg);
	void addInvitation(int fd);
	bool isInvited(int fd);
	void removeInvitation(int fd);
};


