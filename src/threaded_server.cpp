/*---------------------------------------------------------------------------------------
--	SOURCE :		threaded_server.cpp
--
--	PROGRAM:		threaded_server.out
--
--	FUNCTIONS:		int main (int argc, char **argv)
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
--	The program will create a server that handles each connection using one socket.
-- 	The program gathers data for each of the sessions and saves them into a csv file.
---------------------------------------------------------------------------------------*/

#include "threaded_server.h"

int main (int argc, char **argv)
{

	int	port;
	pthread_t updateThread;
	ofstream outfile;

	switch(argc)
	{
		case 1:
			port = SERVER_TCP_PORT;	// Use the default port
		break;
		case 2:
			port = atoi(argv[1]);	// Get user specified port
		break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(EXIT_ERROR);
	}

	//instantiate struct
	struct threaded_utilities *thread_utils = (struct threaded_utilities*) malloc(sizeof(struct threaded_utilities));

	outfile.open(STATS_FILE, ios::out);

	//set data
	thread_utils->client_peak = 0;
	thread_utils->client_current = 0;
	thread_utils->client_total = 0;
	thread_utils->flag = TRUE;
	thread_utils->outfile = &outfile;

	if (!thread_utils->outfile->is_open())
  	{
    	sys_fatal("Can't open outfile");
  	}

  	*thread_utils->outfile << " Client ( IP Address ), Requests ( Send/Receive ), Total Payload ( Bytes ), Serving Time ( ms )"<< endl;
	
  	//Creating Semaphore and mutex
	thread_utils->semaphore = initsem((key_t) SEMAPHORE_KEY);
	SignalSem(thread_utils->semaphore, STARTING_THREADS);

	thread_utils->mutex = initsem((key_t) MUTEX_KEY);
	SignalSem(thread_utils->mutex, 1);

	//create socket
	thread_utils->listening_socket = start_server(port, false);

	//Setting a limit for connections
	for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
            thread_utils->client[i] = -1;
    }

	if((pthread_create(&updateThread, NULL, printupdate, (void*) thread_utils)) != 0)
    {
        sys_fatal("Error creating update thread");
    }

	// Create threads to listen for connections
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

	return EXIT_SUCCESS;
}

void* client_thread(void* ptr)
{
	struct threaded_utilities* thread_utils = (struct threaded_utilities*) ptr;
	unsigned int client_len;
	int new_sd, bytes_to_read, bytes_read;
	struct sockaddr_in client;
	char buf[BUFLEN];
	int flag = 0;
	int payload = 0;
	int requests = 0;

	client_len= sizeof(client);

	//accept new connection
	if ((new_sd = accept (thread_utils->listening_socket, (struct sockaddr *)&client, &client_len)) == -1)
	{
		perror("Error accepting client\n");
		flag = 1;
	}	

	//if error
	if (flag)
	{
		return 0;
	}

	struct timeval startingtime, endingtime;
	gettimeofday(&startingtime, 0);

	//free semaphore
	SignalSem(thread_utils->semaphore, 1);
	int client_cursor;

	//check for empty descriptors

	for (client_cursor = 0; client_cursor < MAX_CONNECTIONS; client_cursor++)
    {
   		if (thread_utils->client[client_cursor] < 0)
    	{
        	thread_utils->client[client_cursor] = new_sd;

        	if(client_cursor > thread_utils->maxi)
        	{
        		thread_utils->maxi = client_cursor;
        	}       	

        	break;
    	}

		client_cursor++;

    	if(client_cursor == MAX_CONNECTIONS)
    	{
    		printf("Server at full capacity\n");
			return 0;
    	}

    	client_cursor--;

    }

    //update stats
	thread_utils->client_current++;
	thread_utils->client_total++;

	if(thread_utils->client_current > thread_utils->client_peak)
	{
		int peak = thread_utils->client_current;
		thread_utils->client_peak = peak;
	}

	bytes_to_read = BUFLEN;

	//read requests
	while ((bytes_read = recv(new_sd, buf, bytes_to_read, 0)) > 0)
	{
		payload += bytes_read;
		requests++;
		send(new_sd, buf, bytes_read, 0);
	}

	gettimeofday(&endingtime, 0);
	close(new_sd);

	double total_time = ((endingtime.tv_sec - startingtime.tv_sec) *  1000000 + endingtime.tv_usec - startingtime.tv_usec) / 1000;
	//update stats
	thread_utils->client_current--;
	thread_utils->client[client_cursor] = -1;

	WaitSem(thread_utils->mutex);
	*thread_utils->outfile << inet_ntoa(client.sin_addr) << "," << requests << "," << payload << "," <<  total_time << endl;
	SignalSem(thread_utils->mutex, 1);

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

            close(thread_utils->listening_socket);

            sleep(1);

			for(int i = 0; i <= thread_utils->maxi; i++)
			{
				if(thread_utils->client[i] > 0)
				{
					close(thread_utils->client[i]);
				}
			}

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