CC=clang++
CFLAGS=-std=c++11 -l pthread
COMPILE=$(CC) $(CFLAGS)

all: client server

server: server.cpp filerequest.cpp
	$(COMPILE) server.cpp filerequest.cpp -o server

client: client.cpp
	$(COMPILE) client.cpp -o client

clean:
	rm server client
