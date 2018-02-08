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
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include "commondefs.h"

void send_syn(int sockfd, struct sockaddr_in *remote_addr);
bool receive_synack(int sockfd, struct sockaddr_in *remote_addr);
void send_req(int sockfd, struct sockaddr_in *remote_addr, 
              const char *filename);
bool receive_reqack(int sockfd, struct sockaddr_in *remote_addr);
bool receive_pack(int sockfd, struct sockaddr_in *remote_addr, 
                  std::ostream& os);
void send_packack(int sockfd, struct sockaddr_in *remote_addr, int packet_num);


bool is_synack(char c);
bool is_reqack(char c);
bool is_pack(char c);
bool is_close(char c);

bool hostname_to_ip(char* str, int &result);

std::string ip_to_string(int ip);
#endif
