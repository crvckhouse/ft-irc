#include <iostream>
#include <vector> //-> for vector
#include <sys/socket.h> //-> for socket()
#include <sys/types.h> //-> for socket()
#include <netinet/in.h> //-> for sockaddr_in
#include <fcntl.h> //-> for fcntl()
#include <unistd.h> //-> for close()
#include <arpa/inet.h> //-> for inet_ntoa()
#include <poll.h> //-> for poll()
#include <csignal> //-> for signal()
#include <string>
#include <sstream>
#include <cerrno>
#include <stdexcept>


class Server
{
	friend class Channel;
private:
	static volatile sig_atomic_t _running;
	int _serverFd;
	const int _port;
	const std::string _passwd;
	int _clientFD;
	std::vector<struct pollfd> _pollFds;
	std::vector<struct client> _clients;
	std::vector<class Channel> _Channels;
	
	int createSocket();
	void bindSocket();
	void Listener();
	int clientAccept();
	void setSocketOptions();
	void setupPoll();
	void run();
	void sendMsg(int clientFD, std::string msg);
	void sendNumber(int clientFD, int code,
    std::string nick, std::string params,
    std::string text);
	void parseCmd(std::string cmd,int clientFD);
	void handleJoin(client *client, std::string channelName);
	void handlePart(client *client, std::string channelName);
	void handlePrivmsg(client *client, std::string msg);
	void handleKick(client *client, std::string argument);
	void handleInvite(client *client, std::string argument);
	void handleTopic(client *client, std::string argument);
	void handleMode(client *client, std::string argument);
	void handleNotice(client *client, std::string argument);
	void handleQuit(client *client, std::string argument);	
	void handleNames(client *client, std::string argument);
	void tryRegister(client *client);
	void handleClose(client *client);
	void removeClient(int fd);
	client *findClient(int fd);
	void enablePollout(int fd);
	void disablePollout(int fd);
	static void handleSignal(int signal);
	void cleanup();
public:
	Server(const int port, const std::string passwd);
	~Server();
};
