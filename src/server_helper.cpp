/*---------------------------------------------------------------------------------------
--	SOURCE :		server_helper.cpp
--
--	PROGRAM:		
--
--	FUNCTIONS:		int start_server(int port, bool nonblocking)
--					int start_client(char* address, int port, bool nonblocking)
--					void sys_fatal(const char* error)
--					void signal_callback_handler(int signum)
--
--	DATE:			February 15, 2016
--
--	REVISIONS:		
--
--	DESIGNERS:		Manuel Gonzales
--
--	PROGRAMMERS:		Manuel Gonzales
--
--	NOTES:
--	Set of helper functions to create sockets for servers and clients
-- 	Also functions to output errors and handle signals.
---------------------------------------------------------------------------------------*/

#include "server_helper.h"

int start_server(int port, bool nonblocking)
{
	int	sd;
	struct	sockaddr_in server;

	// Create a stream socket
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		sys_fatal ("Can't create a socket");
	}

	// Set SO_REUSEADDR
	int optval = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0)
    {
        sys_fatal ("Error Setting SO_REUSEADDR");
    }


    // Set SO_LINGER
    struct linger so_linger;
    memset(&so_linger, 0, sizeof(linger));
    so_linger.l_onoff = TRUE;
	so_linger.l_linger = 0;

	if (setsockopt(sd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof (so_linger)) < 0)
    {
        sys_fatal ("Error Setting SO_LINGER");
    }

    if(nonblocking)
    {
    	if (fcntl (sd, F_SETFL, O_NONBLOCK | fcntl (sd, F_GETFL, 0)) == -1)
    	{
			sys_fatal("Error Setting nonblocking");
		}
    }

	// Bind an address to the socket
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client

	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		sys_fatal("Can't bind name to socket");
	}
	

	if(listen(sd, SOMAXCONN) == -1)
	{
		sys_fatal("Can't set Listen");
	}

	return sd;
}

int start_client(char* address, int port, bool nonblocking)
{
	int	sd;
	struct	sockaddr_in server;
	struct hostent	*hp;

	// Create a stream socket
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		sys_fatal ("Can't create a socket");
	}

	// Set SO_REUSEADDR
	int optval = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0)
    {
        sys_fatal ("Error Setting SO_REUSEADDR");
    }


    // Set SO_LINGER
    struct linger so_linger;
    memset(&so_linger, 0, sizeof(linger));
    so_linger.l_onoff = TRUE;
	so_linger.l_linger = 0;

	if (setsockopt(sd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof (so_linger)) < 0)
    {
        sys_fatal ("Error Setting SO_LINGER");
    }

    if(nonblocking)
    {
    	if (fcntl (sd, F_SETFL, O_NONBLOCK | fcntl (sd, F_GETFL, 0)) == -1)
    	{
			sys_fatal("Error Setting nonblocking");
		}
    }

	//Set Socket
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	if ((hp = gethostbyname(address)) == NULL)
	{
		sys_fatal("Unknown server address\n");
	}

	bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

	if (connect (sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		return 0;
	}
	return sd;
}

//print perror and exit
void sys_fatal(const char* error)
{
    perror(error);
    exit(EXIT_ERROR);
}

//print caught signal num
void signal_callback_handler(int signum)
{
	printf("Received Signal: %d \n", signum);
}