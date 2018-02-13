CC=g++
CFLAGS=-std=c++11 -Wall
MULTITHREAD=-pthread
TESTFILE=./commondefs.h
COMPILE=$(CC) $(CFLAGS)

all: client server

client: common.o clientmain.o client.o
	$(COMPILE) client.o common.o clientmain.o -o client

server: server.o common.o filerequest.o
	$(COMPILE) $(MULTITHREAD) server.o common.o filerequest.o -o server

server.o: server.cpp 
	$(COMPILE) -c server.cpp

filerequest.o: filerequest.cpp
	$(COMPILE) -c filerequest.cpp

clientmain.o: clientmain.cpp
	$(COMPILE) -c clientmain.cpp

client.o: client.cpp 
	$(COMPILE) -c client.cpp

garbage_client.o: garbage_client.cpp
	$(COMPILE) -c garbage_client.cpp

garbage: garbage_client.o client.o common.o
	$(COMPILE) $(MULTITHREAD) garbage_client.o client.o common.o -o garbage

common.o: common.cpp
	$(COMPILE) -c common.cpp

clean:
	rm server client garbage *.o

test: client server
	./server 5954 & > /dev/null 2> /dev/null
	./client localhost 5954 $(TESTFILE)
	pkill server

stest: server garbage
	./server 5954 > /dev/null &
	./garbage localhost 5954 $(TESTFILE) 2> /dev/null
	pkill server
