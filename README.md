*This project has been created as part of the 42 curriculum by ilsedjal, garivoir.*

DESCRIPTION

This project is an IRC server written in C++98 as part of the 42 curriculum.
It allows multiple clients to connect simultaneously and communicate through
channels or private messages using the IRC protocol.

The server supports client registration, channel creation and management,
operator privileges, channel modes, invitations, kicks, topics, private
messages and notices. Network communication is handled with non-blocking
sockets and poll function.

INSTRUCTIONS

### Compilation

The project requires a C++98-compatible compiler, `make`, and an IRC client
such as Irssi.

make

To clean the generated files:

make clean
make fclean

### Starting the server

Start the server with a port and a connection password:

./ircserv <port> <password>


The port must be between `1024` and `65535`. The password must not contain spaces.
For example:

./ircserv 6667 password


### Connecting with Irssi

In another terminal, start Irssi:

irssi

Then connect to the server:

/connect 127.0.0.1 6667 password


### Main commands

/join #channel
/part #channel
/privmsg #channel :Hello everyone
/notice #channel :This is a notice
/topic #channel :Channel topic
/names #channel
/invite nickname #channel
/kick #channel nickname

Channel operators can also use channel modes:

/mode #channel +i // invite only channel
/mode #channel -i // stop invite only
/mode #channel +k password
/mode #channel -k
/mode #channel +l limit users
/mode #channel -l
/mode #channel +o // put new operator
/mode #channel -o // delete operator

RESOURCES

https://github-wiki-see.page/m/42-serv/ft_irc/wiki
https://www.youtube.com/watch?v=D26sUZ6DHNQ
https://www.ibm.com/docs/fr/i/7.6.0?topic=designs-using-poll-instead-select
https://usr.lmf.cnrs.fr/~kn/files/manpages-fr/man2/recv.2.html
https://www.youtube.com/watch?v=cNdlrbZSkyQ

IA has been used for repetitive tasks, and also for debugging, and for some deep understanding of really specific notions.
