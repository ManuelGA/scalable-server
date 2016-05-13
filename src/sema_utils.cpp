/*---------------------------------------------------------------------------------------
--	SOURCE :		sema_utils.cpp
--
--	PROGRAM:		
--
--	FUNCTIONS:		void WaitSem(int sid)
--					void SignalSem(int sid, int value)
--					int initsem (key_t key)
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
--	Set of helper functions to create and manage semaphores
---------------------------------------------------------------------------------------*/

#include "sema_utils.h"

/* Function to decrement the semaphore count by 1*/
void WaitSem(int sid) 
{
    struct sembuf *sembuf_ptr;
        
    sembuf_ptr= (struct sembuf *) malloc (sizeof (struct sembuf *) );
    sembuf_ptr->sem_num = 0;
    sembuf_ptr->sem_op = -1;
    sembuf_ptr->sem_flg = SEM_UNDO;

    if ((semop(sid,sembuf_ptr,1)) == -1)
    {	
	      perror("semop wait error\n");
    }

    free(sembuf_ptr);
}

/* Function to increment the semaphore count by an int value*/
void SignalSem(int sid, int value)
{
	struct sembuf *sembuf_ptr;
        
    sembuf_ptr= (struct sembuf *) malloc (sizeof (struct sembuf *) );
    sembuf_ptr->sem_num = 0;
    sembuf_ptr->sem_op = value;
    sembuf_ptr->sem_flg = SEM_UNDO;

    if ((semop(sid,sembuf_ptr,1)) == -1)
    {
	      perror("semop signal error\n");
    }

    free(sembuf_ptr);
}

/*function to create the semaphore (if for some reason this is nor working you can take the IPC_EXCL flag away so that it can create semaphores even if they already exist)*/
int initsem (key_t key)
{
    int sid, status=0;

    if ((sid = semget((key_t)key, 1, 0666|IPC_CREAT|IPC_EXCL)) == -1)
    {
        if (errno == EEXIST)
        {
            sid = semget ((key_t)key, 1, 0);
        }
    }
    else
    {   /* if created */
        status = semctl (sid, 0, SETVAL, 0);
    }

    if ((sid == -1) || status == -1)
    {
        perror ("initsem failed\n");
        return (-1);
    }
    else
    {
        return (sid);
    }
}

