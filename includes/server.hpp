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
	int _port;
	int _clientFD;
	std::vector<struct pollfd> _pollFds;
	std::vector<struct client> _clients;

	
	

	int createSocket();
	void bindSocket();
	void Listener();
	int clientAccept();
	void setSocketOptions();
	void setupPoll();
	void run();
public:
	Server(int port);
	~Server();
};



