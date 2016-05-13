CC = g++
CFLAGS = -g -I -W -Wall -pedantic
LDFLAGS = -pthread	

threaded_server:
	$(CC) $(CFLAGS) $(LDFLAGS) -o ./threaded_server.out ./threaded_server.cpp ./threaded_server.h ./sema_utils.cpp ./sema_utils.h ./server_helper.cpp ./server_helper.h

select_server:
	$(CC) $(CFLAGS) $(LDFLAGS) -o ./select_server.out ./select_server.cpp ./select_server.h ./sema_utils.cpp ./sema_utils.h ./server_helper.cpp ./server_helper.h

epoll_server:
	$(CC) $(CFLAGS) $(LDFLAGS) -o ./epoll_server.out ./epoll_server.cpp ./epoll_server.h ./sema_utils.cpp ./sema_utils.h ./server_helper.cpp ./server_helper.h

super_client:
	$(CC) $(CFLAGS) $(LDFLAGS) -o ./super_client.out ./super_client.cpp ./super_client.h ./sema_utils.cpp ./sema_utils.h ./server_helper.cpp ./server_helper.h

clean: 
	rm -f *.out *.o *.gch