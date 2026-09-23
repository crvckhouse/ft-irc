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
//-------------------------------------------------------//
#define RED "\e[1;31m" //-> for red color
#define WHI "\e[0;37m" //-> for white color
#define GRE "\e[1;32m" //-> for green color
#define YEL "\e[1;33m" //-> for yellow color

class Server
{
private:
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
	void parseCmd(std::string cmd,int clientFD);
	void handleJoin(client *client, std::string channelName);
	void handlePart(client *client, std::string channelName);
public:
	Server(const int port, const std::string passwd);
	~Server();
};



