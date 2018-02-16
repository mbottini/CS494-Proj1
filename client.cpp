#include "client.h"

void send_syn(int sockfd, struct sockaddr_in *dest_addr) {
  char buf = SYN;
  sendto(sockfd, &buf, 1, 0, (struct sockaddr *)dest_addr,
         sizeof(*dest_addr));
}

rec_outcome receive_synack(int sockfd, struct sockaddr_in *dest_addr) {
  socklen_t addrlen = sizeof(*dest_addr);
  char buf[BUFFSIZE];
  int recvlen = recvfrom(sockfd, buf, BUFFSIZE, 0, (struct sockaddr *)dest_addr,
                         &addrlen);
  if (recvlen == 1 && is_synack(*buf)) {
    std::cerr << "Handshake received from port " << ntohs(dest_addr->sin_port)
              << "\n";
    return REC_SUCCESS;
  } 
  if(recvlen == -1) {
    return REC_TIMEOUT;
  }
  else {
    return REC_FAILURE;
  }
}

void send_req(int sockfd, struct sockaddr_in *dest_addr, const char *filename) {
  char buf[BUFFSIZE];
  *buf = REQ;
  std::memcpy(buf + 1, filename, strlen(filename));
  std::cerr << "Requesting " << std::string(buf + 1, strlen(filename)) << "\n";
  sendto(sockfd, buf, 1 + strlen(filename), 0,
         (struct sockaddr *)dest_addr, sizeof(*dest_addr));
}

rec_outcome receive_reqack(int sockfd, struct sockaddr_in *dest_addr) {
  socklen_t addrlen = sizeof(*dest_addr);
  char buf[BUFFSIZE];
  int recvlen = recvfrom(sockfd, buf, BUFFSIZE, 0, (struct sockaddr *)dest_addr,
                         &addrlen);
  if (recvlen == 5 && is_reqack(*buf)) {
    int file_size;
    std::memcpy(&file_size, buf + 1, 4);
    file_size = ntohl(file_size);
    std::cerr << "File exists, and is size " 
              << file_size << "\n";
    return REC_SUCCESS;;
  } 
  if(recvlen == -1) {
    return REC_TIMEOUT;
  }
  else {
    std::cerr << "File cannot be read.\n";
    return REC_FAILURE;
  }
}

void send_packsyn(int sockfd, struct sockaddr_in *dest_addr, 
                  int size) {
  std::cerr << "Sending PACKSYN request for size " << size << " packets.\n";
  char buf[5];
  *buf = PACK | SYN;
  size = htonl(size);
  memcpy(buf + 1, &size, 4);
  sendto(sockfd, buf, 5, 0,
         (struct sockaddr *)dest_addr, sizeof(*dest_addr));
}

rec_outcome receive_pack(int sockfd, struct sockaddr_in *remote_addr, 
                         int *current_packet, std::ostream *os) {
  char buf[BUFFSIZE];
  socklen_t addrlen = sizeof(*remote_addr);
  int packet_num;
  int recvlen = recvfrom(sockfd, buf, BUFFSIZE, 0, 
                         (struct sockaddr *)remote_addr, &addrlen);
  if(recvlen > 5 && is_pack(*buf)) {
    std::memcpy(&packet_num, buf + 1, 4);
    packet_num = ntohl(packet_num);
    if(packet_num != *current_packet) {
      std::cerr << "Wrong packet.\n";
      return REC_WRONGPACKET;
    }
    os->write(buf + 5, recvlen - 5);
    *current_packet = *current_packet + 1;
    return REC_SUCCESS;
  }

  else if(recvlen == -1) {
    return REC_TIMEOUT;
  }

  else if(recvlen >= 1 && is_close(*buf)) {
    std::cerr << "Received CLOSE.\n";
    return REC_FAILURE;
  }

  return REC_FAILURE;
}

void send_packack(int sockfd, struct sockaddr_in *dest_addr, 
                  int *packet_num) {
  unsigned int pack_num_network;
  char buf[5];
  *buf = PACK | ACK;
  pack_num_network = htonl(*packet_num - 1);
  std::memcpy(buf + 1, &pack_num_network, 4);
  sendto(sockfd, buf, 5, 0,
         (struct sockaddr *)dest_addr, sizeof(*dest_addr));
  return;
}

bool is_synack(char c) {
  return c == (SYN | ACK);
}

bool is_reqack(char c) {
  return c == (REQ | ACK);
}

bool is_pack(char c) {
  return c == PACK;
}

bool is_close(char c) {
  return c == CLOSE;
}

bool hostname_to_ip(char* str, char* port, struct sockaddr *dest_addr) {
  struct addrinfo *ai;
  struct addrinfo hints;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; 
  hints.ai_socktype = SOCK_DGRAM;
  bool valid_addr = false;

  std::cerr << "str = " << str << "\n";
  std::cerr << "port = " << port << "\n";
  if(getaddrinfo(str, port, &hints, &ai) == 0) {
    valid_addr = true;
    *dest_addr = *(ai->ai_addr);
    int ip_addr = ntohl(((struct sockaddr_in*)dest_addr)->sin_addr.s_addr);
    std::cerr << "Found IP address: " << ip_to_string(ip_addr) << "\n";
    freeaddrinfo(ai);
  }

  return valid_addr;
}
