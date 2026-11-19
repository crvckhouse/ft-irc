#include "../includes/server.hpp"
#include "../includes/client.hpp"
#include "../includes/channel.hpp"
#include <cerrno>
#include <cstring>
#include <stdexcept>

void Server::sendNumber(int clientFD, int code,
                        std::string nickname, std::string params, std::string text)
{
    std::ostringstream response;
    response << ":ft_irc ";

    if (code < 100)
        response << "0";
    if (code < 10)
        response << "0";

    response << code << " " << nickname;

    if (!params.empty())
        response << " " << params;

    if (!text.empty())
        response << " :" << text;

    response << "\r\n";
    sendMsg(clientFD, response.str());
}

client *Server::findClient(int fd)
{
    for (std::vector<client>::iterator it = _clients.begin();
         it != _clients.end(); ++it)
    {
        if (it->clientFD == fd)
            return &(*it);
    }
    return NULL;
}

void Server::sendMsg(int clientFD, std::string msg)
{
    client *recipient = findClient(clientFD);
    if (recipient == NULL)
        return;
    if (!recipient->outputBuffer.empty())
    {
        recipient->outputBuffer += msg;
        enablePollout(clientFD);
        return;
    }

    ssize_t bytesSent = send(clientFD, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    if (bytesSent == static_cast<ssize_t>(msg.size()))
        return;
    if (bytesSent == -1)
    {
        if (errno == EINTR)
        {
            recipient->outputBuffer += msg;
            enablePollout(clientFD);
            return;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            recipient->outputBuffer += msg;
            enablePollout(clientFD);
            return;
        }
        if (errno == EPIPE || errno == ECONNRESET)
        {
            std::cout << "Client connection closed while sending" << std::endl;
            return;
        }
        std::cout << "send() error: " << strerror(errno) << std::endl;
        return;
    }
    if (bytesSent > 0 &&
        bytesSent < static_cast<ssize_t>(msg.size()))
    {
        recipient->outputBuffer += msg.substr(bytesSent);
        enablePollout(clientFD);
        std::cout << "Partial send: " << bytesSent << " of "
                  << msg.size() << " bytes" << std::endl;
    }
}
void Server::handleClose(client *client)
{
    client->buffer.clear();
    shutdown(client->clientFD, SHUT_RDWR);
}

void Server::removeClient(int fd)
{
    client *clientToRemove = NULL;
    for (std::vector<client>::iterator it = _clients.begin();
         it != _clients.end(); ++it)
    {
        if (it->clientFD == fd)
        {
            clientToRemove = &(*it);
            break;
        }
    }
    if (clientToRemove == NULL)
        return;

    handleQuit(clientToRemove, "");

    for (std::vector<Channel>::iterator it = _Channels.begin();
         it != _Channels.end();)
    {
        it->removeInvitation(fd);
        ++it;
    }

    close(fd);
    for (std::vector<struct pollfd>::iterator it = _pollFds.begin();
         it != _pollFds.end(); ++it)
    {
        if (it->fd == fd)
        {
            _pollFds.erase(it);
            break;
        }
    }
    for (std::vector<client>::iterator it = _clients.begin();
         it != _clients.end(); ++it)
    {
        if (it->clientFD == fd)
        {
            _clients.erase(it);
            break;
        }
    }
}