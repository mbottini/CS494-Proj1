CC=clang++
CFLAGS=-std=c++11 -l pthread
COMPILE=$(CC) $(CFLAGS)

all: client server

server: server.cpp filerequest.cpp common.o
	$(COMPILE) common.o server.cpp filerequest.cpp -o server

client: client.cpp common.o
	$(COMPILE) common.o client.cpp -o client

common.o:
	$(COMPILE) -c common.cpp

clean:
	rm server client
