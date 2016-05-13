/*---------------------------------------------------------------------------------------
--	SOURCE:		server_helper.h 
--
--	PROGRAM:		
--
--	FUNCTIONS:		
--
--	DATE:			February 15, 2016
--
--	REVISIONS:		(Date and Description)
--
--
--	DESIGNERS:		Manuel Gonzales
--
--	PROGRAMMERS:		Manuel Gonzales
--
--	NOTES:
--	Header file for server helper.
---------------------------------------------------------------------------------------*/

#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H

#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>

#define SERVER_TCP_PORT 8005
#define BUFLEN	512

#define TRUE	1
#define EXIT_ERROR 1
#define EXIT_SUCCESS 0

int start_server(int port, bool nonblocking);
int start_client(char* address, int port, bool nonblocking);
void sys_fatal(const char* error);
void signal_callback_handler(int signum);

#endif