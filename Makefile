CC=clang++
CFLAGS=-std=c++11 -l pthread
COMPILE=$(CC) $(CFLAGS)

all: client server

client: client.o common.o
	$(COMPILE) client.o common.o -o client

server: server.o common.o filerequest.o
	$(COMPILE) server.o common.o filerequest.o -o server

server.o: server.cpp 
	$(COMPILE) -c server.cpp

filerequest.o:
	$(COMPILE) -c filerequest.cpp

client.o: client.cpp 
	$(COMPILE) -c client.cpp

common.o: common.cpp
	$(COMPILE) -c common.cpp

clean:
	rm server client *.o
