/*---------------------------------------------------------------------------------------
--	SOURCE :		select_server.cpp
--
--	PROGRAM:		select_server.out
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
--	The program will create a server that handles connections using the select call and
--	several number of processes
-- 	The program gathers data for each of the sessions and saves them into a csv file.
---------------------------------------------------------------------------------------*/

#include "select_server.h"

int main (int argc, char **argv)
{
	threaded_utilities* thread_utils;
	pid_t pid;

	//set shared memory
    thread_utils = (threaded_utilities*) mmap(NULL, sizeof(thread_utils), PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

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
	thread_utils->listening_socket = start_server(port, true);

	if((pthread_create(&updateThread, NULL, printupdate, (void*) thread_utils)) != 0)
    {
        sys_fatal("Error creating update thread");
    }

	//create processes to listen for connections
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

void* client_thread(void * ptr)
{

	int maxi, nready, bytes_read, bytes_to_read;
	int new_sd, sockfd, maxfd, client[FD_SETSIZE];
	unsigned int client_len;
	struct sockaddr_in client_addr;
	char buf[BUFLEN];

	map<int, client_data> clients;
	threaded_utilities* thread_utils = (threaded_utilities*) ptr;

   	fd_set rset, allset;

   	int stop_flag = 0;
	int flag = 0;
	int extra_process = 0;

	struct timeval timeout;

	maxfd = thread_utils->listening_socket;
	maxi = -1;

	for (int i = 0; i < FD_SETSIZE; i++)
    {
         client[i] = -1;
    }

   	FD_ZERO(&allset);
   	FD_SET(thread_utils->listening_socket, &allset);


   	while (thread_utils->flag)
	{
   		rset = allset;               // structure assignment

   		timeout.tv_sec = TIMEOUT_SECONDS;
		timeout.tv_usec = 0;

		nready = select(maxfd + 1, &rset, NULL, NULL, &timeout);

		//if error or timeout
		if(nready <= 0)
		{
			for (int client_cursor = 0; client_cursor < maxi; client_cursor++)
		    {

		    	if(client[client_cursor] > 0)
		    	{
			    	close(client[client_cursor]);
			    	client[client_cursor] = -1;
			        thread_utils->client_current--;
		    	}
		    }

			if(errno == EBADF || nready == 0)
			{			    
			    FD_ZERO(&allset);
			    FD_SET(thread_utils->listening_socket, &allset);
			    stop_flag = 0;
			}
			else
			{
				SignalSem(thread_utils->semaphore, 1);
				clients.clear();
				sys_fatal("Error in epoll_wait!");
			}
		}
		else
		{			
      		if (FD_ISSET(thread_utils->listening_socket, &rset)) // new client connection
      		{
      			
      			if(!stop_flag)
				{
					client_len = sizeof(client_addr);

					if ((new_sd = accept(thread_utils->listening_socket, (struct sockaddr *) &client_addr, &client_len)) == -1)
					{
						if(errno != EAGAIN)
						{
							sys_fatal("error accepting clients");
						}

						flag = 1;
					}
					
					if(!flag)
					{

						if (fcntl (new_sd, F_SETFL, O_NONBLOCK | fcntl (new_sd, F_GETFL, 0)) == -1)
				    	{
							sys_fatal("Error Setting nonblocking");
						}

						int client_cursor;

						//look for space
						for (client_cursor = 0; client_cursor < FD_SETSIZE; client_cursor++)
					    {
					   		if (client[client_cursor] < 0)
					    	{
					        	client[client_cursor] = new_sd;
					        	stop_flag = 0;
								break;
					        }

					        client_cursor++;

					        if(client_cursor == FD_SETSIZE)
					        {
					        	printf("Process at full capacity\n");

					        	if(!extra_process)
					        	{
					        		SignalSem(thread_utils->semaphore, 1);
					        		extra_process = 1;
					        	}

					        	stop_flag = 1;
					        }

					        client_cursor--;
					    }

					    if(!stop_flag)
					    {

							FD_SET (new_sd, &allset);     // add new descriptor to set

						    thread_utils->client_current++;
						    thread_utils->client_total++;

							if(thread_utils->client_current > thread_utils->client_peak)
							{
								int peak = thread_utils->client_current;
								thread_utils->client_peak = peak;
							}

							if (new_sd > maxfd)
							{
								maxfd = new_sd;	// for select
							}

							if (client_cursor > maxi)
							{
								maxi = client_cursor;	// new max index in client[] array
							}

							client_data client_dat = {};

							//set data
							strncpy(client_dat.address, inet_ntoa(client_addr.sin_addr), ADDRESS_SIZE -1);
							client_dat.payload = 0;
							client_dat.requests = 0;
							gettimeofday(&client_dat.startingtime, 0);

							clients[client_cursor] = client_dat;
						}

						if (--nready <= 0)
						{
							continue;	// no more readable descriptors
						}
					}
				}

				flag = 0;
		     	
	     	}

			for (int client_cursor = 0; client_cursor <= maxi; client_cursor++)	// check all clients.at for data
	     	{
				if ((sockfd = client[client_cursor]) < 0)
				{
					continue;
				}

				if (FD_ISSET(sockfd, &rset))
         		{
         			
					bytes_to_read = BUFLEN;

					//read requests
					while ((bytes_read = recv(sockfd, buf, bytes_to_read, 0)) > 0)
					{

						clients[client_cursor].payload += bytes_read;
						clients[client_cursor].requests++;
						send(sockfd, buf, bytes_read, MSG_NOSIGNAL);
					}

					if (bytes_read == 0) // connection closed by client
	            	{
	            		gettimeofday(&clients[client_cursor].endingtime, 0);

						thread_utils->client_current--;

						close(sockfd);
						FD_CLR(sockfd, &allset);
	               		
	               		stop_flag = 0;

	               		double total_time = ((clients[client_cursor].endingtime.tv_sec - clients[client_cursor].startingtime.tv_sec) *  1000000 + clients[client_cursor].endingtime.tv_usec - clients[client_cursor].startingtime.tv_usec) / 1000;
						
						//update stats
						WaitSem(thread_utils->mutex);
						*thread_utils->outfile << clients[client_cursor].address << "," << clients[client_cursor].requests << "," << clients[client_cursor].payload << "," <<  total_time << endl;
						SignalSem(thread_utils->mutex, 1);

						client[client_cursor] = -1;
	            	}
										            				
					if (--nready <= 0)
					{
	            		break;        // no more readable descriptors
	            	}
				}
     		}
     	}
   	}	

	//close all sockets
   	for (int i = 0; i < FD_SETSIZE; i++)
    {
           if(client[i] > 0)
           {
        		close(client[i]);
           }
    }

	//clear map
    clients.clear();

	exit(EXIT_SUCCESS);	

}

void* printupdate(void * ptr)
{
    string updateflag;
    threaded_utilities* thread_utils = (threaded_utilities*) ptr;

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

            sleep(5);

			semctl(thread_utils->semaphore, 0, IPC_RMID, 0);	
			semctl(thread_utils->mutex, 0, IPC_RMID, 0);						
			thread_utils->outfile->close();
			free(thread_utils);
			
            exit(EXIT_SUCCESS);
        }
    }

    return EXIT_SUCCESS;

}