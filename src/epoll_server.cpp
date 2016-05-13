/*---------------------------------------------------------------------------------------
--	SOURCE :		epoll_server.cpp
--
--	PROGRAM:		epoll_server.out
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
--	The program will create a server that handles connections using the epoll call and
--	several number of processes
-- 	The program gathers data for each of the sessions and saves them into a csv file.
---------------------------------------------------------------------------------------*/

#include "epoll_server.h"

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
	int num_fds, fd_new, epoll_fd, bytes_to_read, bytes_read;
	static struct epoll_event events[EPOLL_QUEUE_LEN], event;
	struct sockaddr_in client_addr;
	unsigned int client_len;
	char buf[BUFLEN];
	int client[MAX_CONNECTIONS];

	for (int i = 0; i < FD_SETSIZE; i++)
    {
         client[i] = -1;
    }

	map<int, client_data> clients;
	threaded_utilities* thread_utils = (threaded_utilities*) ptr;

   	int stop_flag = 0;
	int extra_process = 0;
	int maxi = -1;

	int timeout;

	if((epoll_fd = epoll_create(EPOLL_QUEUE_LEN)) == -1)
	{
		sys_fatal("Error Creating Epoll");
	}
	
	// Add the server socket to the epoll event loop
	event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
	event.data.fd = thread_utils->listening_socket;

	if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, thread_utils->listening_socket, &event) == -1)
	{ 
		sys_fatal("Error Modifying Epoll");
	}
    	
	// Execute the epoll event loop
    while (thread_utils->flag) 
	{
		timeout = TIMEOUT_MILSECONDS;

		num_fds = epoll_wait (epoll_fd, events, EPOLL_QUEUE_LEN, timeout);
	
		//if error or timeout
		if(num_fds <= 0)
		{
			for (int client_cursor = 0; client_cursor < maxi; client_cursor++)
		    {

		    	if(client[client_cursor] > 0)
		    	{
		    		if (epoll_ctl (epoll_fd, EPOLL_CTL_DEL, client[client_cursor], &event) == -1)
					{
						sys_fatal ("epoll_ctl");
					}

					clients.erase(client[client_cursor]);
			    	close(client[client_cursor]);

			    	client[client_cursor] = -1;
			        thread_utils->client_current--;
		    	}
		    }

			if(num_fds == 0 || errno == EBADF)
			{
				stop_flag = 0;
			}	
			else
			{
				SignalSem(thread_utils->semaphore, 1);
				clients.clear();
				sys_fatal("Error in epoll_wait!");
			}			
		}

		
		for (int client_cursor = 0; client_cursor < num_fds; client_cursor++) 
		{
			//if error or reset received
    		if (events[client_cursor].events & EPOLLHUP)
			{
				int sockfd = events[client_cursor].data.fd;

				bytes_to_read = BUFLEN;

			    gettimeofday(&clients[sockfd].endingtime, 0);

				thread_utils->client_current--;             		

           		double total_time = ((clients[sockfd].endingtime.tv_sec - clients[sockfd].startingtime.tv_sec) *  1000000 + clients[sockfd].endingtime.tv_usec - clients[sockfd].startingtime.tv_usec) / 1000;
				
				//update stats
				WaitSem(thread_utils->mutex);
				*thread_utils->outfile << clients[sockfd].address << "," << clients[sockfd].requests << "," << clients[sockfd].payload << "," <<  total_time << endl;
				SignalSem(thread_utils->mutex, 1);

				if (epoll_ctl (epoll_fd, EPOLL_CTL_DEL, sockfd, &event) == -1)
				{
					sys_fatal ("epoll_ctl");
				}

				clients.erase(sockfd);
				close(sockfd);
				
				stop_flag = 0;				

				continue;
    		}
	
			//if error received
    		if (events[client_cursor].events & EPOLLERR) 
			{
				close(events[client_cursor].data.fd);
				continue;
			}
    		assert (events[client_cursor].events & EPOLLIN);

    		//Server is receiving a connection request
    		if (events[client_cursor].data.fd == thread_utils->listening_socket) 
			{
				if(!stop_flag)
				{
					client_len = sizeof (client_addr);
					if((fd_new = accept (thread_utils->listening_socket, (struct sockaddr*) &client_addr, &client_len)) == -1)
					{
						if(errno != EAGAIN)
						{
							sys_fatal("error accepting clients");
						}

						continue;
					}
						
					// Make the fd_new non-blocking					
					if (fcntl (fd_new, F_SETFL, O_NONBLOCK | fcntl (fd_new, F_GETFL, 0)) == -1)
			    	{
						sys_fatal("Error Setting nonblocking");
					}
					
					// Add the new socket descriptor to the epoll loop
					event.data.fd = fd_new;

					if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd_new, &event) == -1)
					{
						if(errno == ENOSPC)
						{
							if(!extra_process)
				        	{
				        		SignalSem(thread_utils->semaphore, 1);
				        		extra_process = 1;
				        	}

				        	stop_flag = 1;
						}
						else
						{
							sys_fatal ("epoll_ctl");
						}
					}

					thread_utils->client_current++;
					thread_utils->client_total++;

					int capacity = 0;

					for (capacity = 0; capacity < maxi; capacity++)
				    {

				    	if(client[capacity] < 0)
				    	{
				    		client[capacity] = fd_new;

				    		if(capacity > maxi)
				    		{
				    			maxi = capacity;
				    		}

				    		break;
				    	}
 
				    }

				    if (capacity == MAX_CONNECTIONS - 1)
				    {
				    	if(!extra_process)
			        	{
			        		SignalSem(thread_utils->semaphore, 1);
			        		extra_process = 1;
			        	}

			        	stop_flag = 1;
				    }

					if(thread_utils->client_current > thread_utils->client_peak)
					{
						int peak = thread_utils->client_current;
						thread_utils->client_peak = peak;
					}

					client_data client_dat = {};

					strncpy(client_dat.address, inet_ntoa(client_addr.sin_addr), ADDRESS_SIZE -1);
					client_dat.payload = 0;
					client_dat.requests = 0;
					gettimeofday(&client_dat.startingtime, 0);

					clients[fd_new] = client_dat;

					continue;
				}			
					
	    	}
	    	else //data received.
			{
				int sockfd = events[client_cursor].data.fd;

				bytes_to_read = BUFLEN;

				//read requests
				while ((bytes_read = recv(sockfd, buf, bytes_to_read, 0)) > 0)
				{

					clients[sockfd].payload += bytes_read;
					clients[sockfd].requests++;
					send(sockfd, buf, bytes_read, MSG_NOSIGNAL);
				}

				if (bytes_read == 0) // connection closed by client
            	{

            		gettimeofday(&clients[sockfd].endingtime, 0);

					thread_utils->client_current--;             		

               		double total_time = ((clients[sockfd].endingtime.tv_sec - clients[sockfd].startingtime.tv_sec) *  1000000 + clients[sockfd].endingtime.tv_usec - clients[sockfd].startingtime.tv_usec) / 1000;
					
					//update stats
					WaitSem(thread_utils->mutex);
					*thread_utils->outfile << clients[sockfd].address << "," << clients[sockfd].requests << "," << clients[sockfd].payload << "," <<  total_time << endl;
					SignalSem(thread_utils->mutex, 1);

					clients.erase(sockfd);
					close(sockfd);

					if (epoll_ctl (epoll_fd, EPOLL_CTL_DEL, sockfd, &event) == -1)
					{
						sys_fatal ("epoll_ctl");
					}

					stop_flag = 0;

					

            	}
	    	}
		}
    }

	//close all sockets
    for (int client_cursor = 0; client_cursor < maxi; client_cursor++)
    {

    	if(client[client_cursor] > 0)
    	{
    		if (epoll_ctl (epoll_fd, EPOLL_CTL_DEL, client[client_cursor], &event) == -1)
			{
				sys_fatal ("epoll_ctl");
			}

			clients.erase(client[client_cursor]);
	    	close(client[client_cursor]);

	    	client[client_cursor] = -1;
	        thread_utils->client_current--;
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