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
rec_outcome receive_synack(int sockfd, struct sockaddr_in *remote_addr);
void send_req(int sockfd, struct sockaddr_in *remote_addr, 
              const char *filename);
rec_outcome receive_reqack(int sockfd, struct sockaddr_in *remote_addr);
void send_packsyn(int sockfd, struct sockaddr_in *dest_addr, 
                  int size);
rec_outcome receive_pack(int sockfd, struct sockaddr_in *remote_addr, 
                         int *current_packet, std::ostream *os);
void send_packack(int sockfd, struct sockaddr_in *remote_addr, int packet_num);


bool is_synack(char c);
bool is_reqack(char c);
bool is_pack(char c);
bool is_close(char c);

std::string ip_to_string(int ip);
bool hostname_to_ip(char* str, char* port, struct sockaddr *dest_addr);
#endif
