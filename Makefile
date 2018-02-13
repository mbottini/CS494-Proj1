CC=clang++
CFLAGS=-std=c++11 
MULTITHREAD=-l pthread
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

shitty_client.o: shitty_client.cpp
	$(COMPILE) -c shitty_client.cpp

shitty: shitty_client.o client.o common.o
	$(COMPILE) $(MULTITHREAD) shitty_client.o client.o common.o -o shitty

common.o: common.cpp
	$(COMPILE) -c common.cpp

clean:
	rm server client *.o

test: client server
	./server 5954 & > /dev/null 2> /dev/null
	./client localhost 5954 $(TESTFILE)
	pkill server

stest: server shitty
	./server 5954 > /dev/null &
	./shitty localhost 5954 $(TESTFILE) 2> /dev/null
	pkill server
