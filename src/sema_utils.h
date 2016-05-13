/*---------------------------------------------------------------------------------------
--	SOURCE:		sema_utils.h 
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
--	Header file for semaphores.
---------------------------------------------------------------------------------------*/

#ifndef SEMA_UTILS_H
#define SEMA_UTILS_H

#include <stdio.h>
#include <sys/sem.h>
#include <wait.h>
#include <stdlib.h>
#include <errno.h>

/*keys for the semaphores*/
#define SEMAPHORE_KEY 300
#define MUTEX_KEY 301

int initsem (key_t key);
void WaitSem(int sid);
void SignalSem(int sid, int value);

#endif