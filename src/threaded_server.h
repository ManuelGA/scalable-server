/*---------------------------------------------------------------------------------------
--	SOURCE:		threaded_server.h 
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
--	Header file for threaded server.
---------------------------------------------------------------------------------------*/

#ifndef THREADED_SERVER_H
#define THREADED_SERVER_H

#include <iostream>
#include <pthread.h>
#include <string>
#include <fstream>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "server_helper.h"
#include "sema_utils.h"

#define MAX_CONNECTIONS 100000
#define STARTING_THREADS 200
#define STATS_FILE "threaded_stats.csv"

using namespace std;

void* client_thread(void* ptr);
void* printupdate( void* ptr);

//for data gathering and control
struct threaded_utilities
{
	int semaphore;
	int mutex;
	int listening_socket;
	int client_peak;
	int client_current;
	int client_total;
	int flag;
	int maxi;
	ofstream *outfile;
	int client[MAX_CONNECTIONS];	
};

#endif