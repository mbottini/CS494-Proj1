#include "client.h"

#define BUFFSIZE 2048
#define TIMEOUT 1000

bool receive_synack(int sockfd, struct sockaddr_in *dest_addr) {
  socklen_t addrlen = sizeof(*dest_addr);
  char buf[BUFFSIZE];
  int recvlen = recvfrom(sockfd, buf, BUFFSIZE, 0, (struct sockaddr *)dest_addr,
                         &addrlen);
  if (recvlen == 1 && is_synack(*buf)) {
    std::cout << "Handshake received from port " << ntohs(dest_addr->sin_port)
              << "\n";
    return true;
  } else {
    return false;
  }
}

void send_syn(int sockfd, struct sockaddr_in *dest_addr) {
  char buf = SYN;
  sendto(sockfd, &buf, 1, 0, (struct sockaddr *)dest_addr,
         sizeof(*dest_addr));
}

void send_req(int sockfd, struct sockaddr_in *dest_addr, char *filename) {
  char buf[BUFFSIZE];
  *buf = REQ;
  std::memcpy(buf + 1, filename, strlen(filename));
  std::cout << "REQ " << std::string(buf + 1, strlen(filename)) << "\n";
  sendto(sockfd, buf, 1 + strlen(filename), 0,
         (struct sockaddr *)dest_addr, sizeof(*dest_addr));
}

bool receive_reqack(int sockfd, struct sockaddr_in *dest_addr) {
  socklen_t addrlen = sizeof(*dest_addr);
  char buf[BUFFSIZE];
  int recvlen = recvfrom(sockfd, buf, BUFFSIZE, 0, (struct sockaddr *)dest_addr,
                         &addrlen);
  if (recvlen == 5 && is_reqack(*buf)) {
    int file_size;
    std::memcpy(&file_size, buf + 1, 4);
    file_size = ntohl(file_size);
    std::cout << "File exists, and is size " 
              << file_size << "\n";
    return true;
  } else if (recvlen == 1 && is_close(*buf)) {
    std::cout << "Got CLOSE message. File cannot be read.\n";
  }
  return false;
}

void send_packsyn(int sockfd, struct sockaddr_in *dest_addr, 
                  int size = BUFFSIZE) {
  char buf[5];
  *buf = PACK | SYN;
  size = htonl(size);
  memcpy(buf + 1, &size, 4);
  sendto(sockfd, buf, 5, 0,
         (struct sockaddr *)dest_addr, sizeof(*dest_addr));
}

bool receive_pack(int sockfd, struct sockaddr_in *dest_addr,
                         std::ostream& os) {
  char buf[BUFFSIZE];
  socklen_t addrlen = sizeof(*dest_addr);
  int packet_num;
  int recvlen = recvfrom(sockfd, buf, BUFFSIZE, 0, 
                         (struct sockaddr *)dest_addr, &addrlen);
  if(recvlen > 5 && is_pack(*buf)) {
    os.write(buf + 5, recvlen - 5);
    std::memcpy(&packet_num, buf + 1, 4);
    packet_num = ntohl(packet_num);
    send_packack(sockfd, dest_addr, packet_num);
  }

  else if(is_close(*buf)) {
    return false;
  }

  return true;
}

void send_packack(int sockfd, struct sockaddr_in *dest_addr, 
                  int packet_num) {
  char buf[5];
  *buf = PACK | ACK;
  packet_num = htonl(packet_num);
  std::memcpy(buf + 1, &packet_num, 4);
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

  std::cout << "str = " << str << "\n";
  std::cout << "port = " << port << "\n";
  if(getaddrinfo(str, port, &hints, &ai) == 0) {
    valid_addr = true;
    *dest_addr = *(ai->ai_addr);
    std::cout << "Found IP address: " << ip_to_string(((struct sockaddr_in*)dest_addr)->sin_addr.s_addr) << "\n";
  }

  freeaddrinfo(ai);
  return valid_addr;
}

std::string ip_to_string(int ip) {
  std::stringstream ss;
  int mask = 0xFF;
  for(int i = 0; i < 4; i++) {
    ss << ((ip >> (4 * i)) & mask);
    ss << ".";
  }

  return ss.str().substr(0, ss.str().length() - 1);
}

// Argv contents:
// 0 : Name of program
// 1 : IP Address (e.g. 192.168.88.254)
// 2 : Port (16-bit integer)
// 3 : File path on the server.

int main(int argc, char **argv) {
  // Server socket.
  struct sockaddr_in dest_addr;
  int dest_ip_addr;
  int dest_port;

  int sock_handle;

  if (argc < 4 || argc > 5) {
    std::cout << "Invalid number of arguments. Exiting.\n";
    exit(1);
  }

  if (!hostname_to_ip(argv[1], argv[2], (struct sockaddr*)&dest_addr)) {
    std::cout << "Invalid IP address. Exiting.\n";
    exit(2);
  }

  if (!(std::stringstream(argv[2]) >> dest_port) || dest_port < 0 ||
      dest_port >= 65536) {
    std::cout << "Invalid port number. Exiting.\n";
    exit(3);
  }

  // Open the socket.
  sock_handle = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_handle < 0) {
    std::cout << "Unable to open socket. Aborting.\n";
    exit(1);
  }

  /*
  // Open the file for writing.
  std::ofstream outfile(argv[4]);
  */

  send_syn(sock_handle, &dest_addr);
  if(!receive_synack(sock_handle, &dest_addr))
    return 0;
  send_req(sock_handle, &dest_addr, argv[3]);
  if(!receive_reqack(sock_handle, &dest_addr)) {
    return 0;
  }
  send_packsyn(sock_handle, &dest_addr);
  while(receive_pack(sock_handle, &dest_addr, std::cout));

  return 0;
}
