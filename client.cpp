#include "client.h"

#define BUFFSIZE 2048
#define TIMEOUT 1000

void receive_synack(int sockfd, struct sockaddr_in *dest_addr) {
  unsigned int addrlen = sizeof(*dest_addr);
  char buf[BUFFSIZE];
  int recvlen = recvfrom(sockfd, buf, BUFFSIZE, 0, (struct sockaddr *)dest_addr,
                         &addrlen);
  if (recvlen == 1 && is_synack(*buf)) {
    std::cout << "Handshake received from port " << ntohs(dest_addr->sin_port)
              << "\n";
  } else {
    std::cout << "Received something else.\n";
  }
}

void send_syn(int sockfd, struct sockaddr_in *dest_addr) {
  char buf = SYN;
  sendto(sockfd, &buf, 1, 0, (struct sockaddr *)dest_addr,
         sizeof(*dest_addr));
}

/*
bool receive_file_size(int sockfd, struct sockaddr_in *dest_addr) {
  unsigned int addrlen = sizeof(*dest_addr);
  char buf[BUFFSIZE];
  int recvlen = recvfrom(sockfd, buf, BUFFSIZE, 0, (struct sockaddr *)dest_addr,
                         &addrlen);
  std::string rec_string = buf;
  if (rec_string.length() > 3 && rec_string.substr(0, 3) == "YES") {
    std::cout << "File is here, and is size " << std::stoi(rec_string.substr(3))
              << "\n";
    return true;
  } else if (rec_string.length() >= 5 && rec_string.substr(0, 5) == "CLOSE") {
    std::cout << "Got CLOSE message. File cannot be read.\n";
    return false;
  }
}

bool request_file(int sockfd, struct sockaddr_in *dest_addr, char *filename) {
  std::string request_str = "GET";
  request_str += filename;
  std::cout << "Sending request " << request_str << "\n";
  sendto(sockfd, request_str.c_str(), request_str.length(), 0,
         (struct sockaddr *)dest_addr, sizeof(*dest_addr));
  receive_file_size(sockfd, dest_addr);
}

bool receive_file_packet(int sockfd, struct sockaddr_in *dest_addr,
                         std::ostream& os) {
  char buf[BUFFSIZE];
  unsigned int addrlen = sizeof(*dest_addr);
  int recvlen = recvfrom(sockfd, buf, BUFFSIZE, 0, (struct sockaddr *)dest_addr,
                         &addrlen);
  std::cout << "Received " << recvlen << " bytes.\n";
  if(std::string(buf, 5) == "CLOSE") {
    std::cout << "Close message received.\n";
    return false;
  }
  os.write(buf, recvlen);
  return true;
}

bool request_file_packet(int sockfd, struct sockaddr_in *dest_addr, 
                         int packet_num, std::ostream& os) {
  std::cout << "Requesting packet " << packet_num << "\n";
  std::string request_str = "PAC" + std::to_string(packet_num);
  sendto(sockfd, request_str.c_str(), request_str.length(), 0,
         (struct sockaddr *)dest_addr, sizeof(*dest_addr));
  return receive_file_packet(sockfd, dest_addr, os);
}

bool request_file_packets(int sockfd, struct sockaddr_in *dest_addr,
                          std::ostream& os) {
  for(int packet_num = 0;
      request_file_packet(sockfd, dest_addr, packet_num, os);
      packet_num++);
  return true;
}
*/

bool is_synack(char c) {
  return c == (SYN | ACK);
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

  if (argc != 5) {
    std::cout << "Invalid number of arguments. Exiting.\n";
    exit(1);
  }

  if (!inet_pton(AF_INET, argv[1], &dest_ip_addr)) {
    std::cout << "Invalid IP address. Exiting.\n";
    exit(2);
  }

  if (!(std::stringstream(argv[2]) >> dest_port) || dest_port < 0 ||
      dest_port >= 65536) {
    std::cout << "Invalid port number. Exiting.\n";
    exit(3);
  }

  // Setting various things in dest_addr.
  std::memset((char *)&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(dest_port);
  dest_addr.sin_addr.s_addr = dest_ip_addr;

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
  receive_synack(sock_handle, &dest_addr);

  return 0;
}
