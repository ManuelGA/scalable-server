/*---------------------------------------------------------------------------------------
--	SOURCE:		super_client.h 
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
--	Header file for client.
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
#include <sys/mman.h>
#include <sys/shm.h>

#include "server_helper.h"
#include "sema_utils.h"

#define STATS_FILE "clientstats.csv"
#define MAX_CONNECTIONS 100000
#define SEM_THREADS 500

using namespace std;

void* client_thread(void* ptr);
void* printupdate( void* ptr);

//for data gathering and control
struct threaded_utilities
{
	int semaphore;
	int mutex;
	int client_peak;
	int client_current;
	int client_total;
	int maxi;
	int flag;
	int	port;
	int requestsnumber;
	int sem_number;
	int clients;
	char* data;
	char* host;
	ofstream *outfile;
	int client[MAX_CONNECTIONS];
};

#endif