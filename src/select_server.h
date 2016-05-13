/*---------------------------------------------------------------------------------------
--	SOURCE:		select_server.h 
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
--	Header file for select server.
---------------------------------------------------------------------------------------*/
#ifndef SELECT_SERVER_H
#define SELECT_SERVER_H

#include <iostream>
#include <pthread.h>
#include <string>
#include <fstream>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <math.h>
#include <map>

#include "server_helper.h"
#include "sema_utils.h"

#define STARTING_THREADS 50
#define TIMEOUT_SECONDS 20
#define ADDRESS_SIZE 20
#define STATS_FILE "select_stats.csv"

using namespace std;

void* client_thread(void * ptr);
void* printupdate(void * ptr);

//for data gathering and control
typedef struct 
{
	int semaphore;
	int mutex;
	int listening_socket;
	int client_peak;
	int client_current;
	int client_total;
	int flag;
	ofstream *outfile;
}threaded_utilities;

//for data gathering
typedef struct 
{
	char address[ADDRESS_SIZE];
	int payload;
	int requests;
	struct timeval startingtime;
	struct timeval endingtime;
}client_data;

#endif