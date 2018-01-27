#ifndef CLIENT_H
#define CLIENT_H
#include <iostream>
#include <sstream>
#include <string> 
#include <cstring>     // memcpy
#include <ostream>
#include <fstream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "commondefs.h"

#define BUFFSIZE 2048

void send_syn(int sockfd, struct sockaddr_in *remote_addr);
void receive_synack(int sockfd, struct sockaddr_in *remote_addr);
void send_req(int sockfd, struct sockaddr_in *remote_addr, 
              const char *filename);
bool receive_reqack(int sockfd, struct sockaddr_in *remote_addr);

bool is_synack(char c);
bool is_reqack(char c);
bool is_close(char c);



#endif
