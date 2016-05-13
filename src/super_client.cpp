/*---------------------------------------------------------------------------------------
--	SOURCE :		super_client.cpp
--
--	PROGRAM:		super_client.out
--
--	FUNCTIONS:		int main (int argc, char **argv)
--					void run_process(void* ptr)
--					void* client_thread(void* ptr)
--					void* printupdate( void* ptr )
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
--	The program will create a set of processes that control threads that will set connection with
--	a specific server.
-- 	The program gathers data for each of the sessions and saves them into a csv file.
---------------------------------------------------------------------------------------*/

#include "super_client.h"

int main (int argc, char **argv)
{

	threaded_utilities* thread_utils;
	pid_t pid;

	//set shared memory
	thread_utils = (threaded_utilities*) mmap(NULL, sizeof(thread_utils), PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);


	pthread_t updateThread;	
	ofstream outfile;

	if(argc != 7 )
	{
		fprintf(stderr, "Usage: %s [host] [port] [number_clients] [number_requests] [number_processes] [string_to_send]\n", argv[0]);
		exit(EXIT_ERROR);
	}

	int	port;
	int requestsnumber;
	char* data;
	char* host;
	int clients;
	int processes;
	
	//set user input
	host = argv[1];
	port = atoi(argv[2]);
	clients = atoi(argv[3]);
	requestsnumber = atoi(argv[4]);
	processes = atoi(argv[5]);
	data = argv[6];

	//instantiate struct

	outfile.open(STATS_FILE, ios::out);

	//set data
	thread_utils->client_peak = 0;
	thread_utils->client_current = 0;
	thread_utils->client_total = 0;
	thread_utils->port = port;
	thread_utils->requestsnumber = requestsnumber;
	thread_utils->clients = clients / processes;
	thread_utils->data = data;
	thread_utils->host = host;
	thread_utils->outfile = &outfile;
	thread_utils->flag = TRUE;
	thread_utils->sem_number = MUTEX_KEY + 1;

	if (!thread_utils->outfile->is_open())
  	{
    	sys_fatal("Can't open outfile");
  	}

  	*thread_utils->outfile << " Requests ( Send/Receive ), Total Payload ( Bytes ), Total Time ( ms )"<< endl;
	
  	//Creating Semaphore and mutex
	thread_utils->semaphore = initsem((key_t) SEMAPHORE_KEY);
	SignalSem(thread_utils->semaphore, processes);


	thread_utils->mutex = initsem((key_t) MUTEX_KEY);
	SignalSem(thread_utils->mutex, 1);

	if((pthread_create(&updateThread, NULL, printupdate, (void*) thread_utils)) != 0)
    {
        sys_fatal("Error creating update thread");
    }

    // for (int i = 0; i < MAX_CONNECTIONS; i++)
    // {
    //         thread_utils->client[i] = -1;
    // }

	// create processes to set connections
	while (thread_utils->flag)
	{
		WaitSem(thread_utils->semaphore);

		if(fork() == 0)
		{
			client_thread((void*) thread_utils);
		}
	}

	while (pid == wait(NULL))
	{
		if(errno == ECHILD)
		{
			break;

		}
	}

	exit(EXIT_SUCCESS);
}

void run_process(void* ptr)
{
	struct threaded_utilities* process_shared = (struct threaded_utilities*) ptr;

	//instantiate struct
	struct threaded_utilities *thread_utils = (struct threaded_utilities*) malloc(sizeof(struct threaded_utilities));

	//set data for threads
	thread_utils->client_peak = process_shared->client_peak;
	thread_utils->client_current = process_shared->client_current;
	thread_utils->client_total = process_shared->client_total;
	thread_utils->flag = TRUE;
	thread_utils->outfile = process_shared->outfile;
	thread_utils->port = process_shared->port;
	thread_utils->requestsnumber = process_shared->requestsnumber;
	thread_utils->data = process_shared->data;
	thread_utils->host = process_shared->host;
	
  	//Creating Semaphore and mutex for threads
	thread_utils->semaphore = initsem((key_t) process_shared->sem_number);
	process_shared->sem_number++;
	SignalSem(thread_utils->semaphore, process_shared->clients);

	thread_utils->mutex = process_shared->mutex;


	//create threads to start connections
	while (thread_utils->flag)
	{

		WaitSem(thread_utils->semaphore);
		pthread_t thread;

		if((pthread_create(&thread, NULL, client_thread, (void*) thread_utils)) != 0)
    	{
        	sys_fatal("Error creating client thread");
   		}

   		pthread_detach(thread);

	}

	semctl(thread_utils->semaphore, 0, IPC_RMID, 0);
	free(thread_utils);

	exit(EXIT_SUCCESS);
}

void* client_thread(void* ptr)
{
	struct threaded_utilities* thread_utils = (struct threaded_utilities*) ptr;

	while(thread_utils->flag)
	{
		int payload = 0;
		int requests = 0;
		int bytes_to_read, n;
		char  *bp, rbuf[BUFLEN];

		//create socket
		int client_socket = start_client(thread_utils->host, thread_utils->port, false);

		if(client_socket == 0)
		{
			return 0;
		}

		//int client_cursor;

		//check for empty descriptors
		// for (client_cursor = 0; client_cursor < MAX_CONNECTIONS; client_cursor++)
	 //    {
	 //   		if (thread_utils->client[client_cursor] < 0)
	 //    	{
	 //        	thread_utils->client[client_cursor] = client_socket;

	 //        	if(client_cursor > thread_utils->maxi)
	 //        	{
	 //        		thread_utils->maxi = client_cursor;
	 //        	}       	

	 //        	break;
	 //    	}

		// 	client_cursor++;

	 //    	if(client_cursor == MAX_CONNECTIONS)
	 //    	{
	 //    		printf("Clients at full capacity\n");
		// 		return 0;
	 //    	}

	 //    	client_cursor--;
	 //    }

	    struct timeval startingtime, endingtime;
		gettimeofday(&startingtime, 0);

	    //update stats
		thread_utils->client_current++;
		thread_utils->client_total++;

		if(thread_utils->client_current > thread_utils->client_peak)
		{
			int peak = thread_utils->client_current;
			thread_utils->client_peak = peak;
		}

		for (int i = 0; i < thread_utils->requestsnumber; i++)
	    {
	    	int size = strlen(thread_utils->data);

	    	if(send(client_socket, thread_utils->data, size, 0) == -1)
	    	{
	    		break;
	    	}
	    	
	    	requests++;
	    	payload += size;

	    	bp = rbuf;
	    	bytes_to_read = size;

			// client makes repeated calls to get data.
			n = 0;
			while ((n = recv (client_socket, bp, bytes_to_read, 0)) < size)
			{
				bp += n;
				bytes_to_read -= n;
			}

			if(n == 0)
			{
				break;
			}

	    }

		gettimeofday(&endingtime, 0);
		close(client_socket);

		double total_time = ((endingtime.tv_sec - startingtime.tv_sec) *  1000000 + endingtime.tv_usec - startingtime.tv_usec) / 1000;
		//update stats
		thread_utils->client_current--;
		//thread_utils->client[client_cursor] = -1;

		WaitSem(thread_utils->mutex);
		*thread_utils->outfile << requests << "," << payload << "," <<  total_time << endl;
		SignalSem(thread_utils->mutex, 1);
	}

	return EXIT_SUCCESS;	

}

void* printupdate( void* ptr )
{
    struct threaded_utilities* thread_utils = (struct threaded_utilities*) ptr;
    string updateflag;

    while(thread_utils->flag)
    {
        getline(cin, updateflag);

        //show stats
        if(strcasecmp(updateflag.c_str(), "show") == 0)
        {
            printf("\n Client Stats: \n");
            printf("Current Clients: %d \n", thread_utils->client_current );
            printf("Clients Peak: %d \n", thread_utils->client_peak );
            printf("Clients Total: %d \n", thread_utils->client_total );
        }

        //close all sockets, free mem
        if(strcasecmp(updateflag.c_str(), "stop") == 0)
        {
            thread_utils->flag = 0;
			
            printf("Shutting server Down... \n");

            sleep(1);

			// for(int i = 0; i <= thread_utils->maxi; i++)
			// {
			// 	if(thread_utils->client[i] > 0)
			// 	{
			// 		close(thread_utils->client[i]);
			// 	}
			// }

			sleep(1);

			semctl(thread_utils->semaphore, 0, IPC_RMID, 0);	
			semctl(thread_utils->mutex, 0, IPC_RMID, 0);						
			thread_utils->outfile->close();

			free(thread_utils);
			
            exit(EXIT_SUCCESS);
        }
    }

    return EXIT_SUCCESS;
}