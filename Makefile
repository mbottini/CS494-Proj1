CC=clang++
CFLAGS=-std=c++11 
MULTITHREAD=-l pthread
COMPILE=$(CC) $(CFLAGS)

all: client server

client: client.o common.o
	$(COMPILE) client.o common.o -o client

server: server.o common.o filerequest.o
	$(COMPILE) $(MULTITHREAD) server.o common.o filerequest.o -o server

server.o: server.cpp 
	$(COMPILE) -c server.cpp

filerequest.o: filerequest.cpp
	$(COMPILE) -c filerequest.cpp

client.o: client.cpp 
	$(COMPILE) -c client.cpp

common.o: common.cpp
	$(COMPILE) -c common.cpp

clean:
	rm server client *.o

test: client server
	./server 5954 &
	./client localhost 5954 /home/mike/stuff.hs
	pkill server
