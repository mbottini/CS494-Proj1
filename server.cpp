#include "server.h"
#include <iostream>
#include <sstream>
#include <thread>

void await_syn(int sockfd, struct sockaddr_in *remote_addr) {
  char buf[BUFFSIZE];
  unsigned int addrlen = sizeof(*remote_addr);
  int recvlen = recvfrom(sockfd, buf, BUFFSIZE, 0, 
                         (struct sockaddr*)remote_addr, &addrlen);
  if(recvlen == 1 && is_syn(buf[0])) {
    std::cout << "Handshake received. Starting new thread.\n";
    struct sockaddr_in current_addr = *remote_addr;
    std::thread t(subordinate_thread, current_addr);
    t.detach();
  }

  else {
    std::cout << "Garbage received. Ignoring.\n";
  }
  return;
}

void subordinate_thread(struct sockaddr_in current_addr) {
  FileRequest fr(current_addr);
  std::cout << "Thread started with FileRequest:\n" << fr << "\n";
  fr.send_synack();
  fr.receive_req();
  fr.open_file();
  fr.send_reqack();
  fr.receive_packsyn();
  std::cout << "Exiting thread.\n";
}

bool is_syn(char c) {
    return c == SYN;
}

/*
void send_close(const struct file_request& fr) {
  char buf[1];
  *buf = CLOSE;
  sendto(fr.sock_fd, buf, 1, 0,
         (struct sockaddr *)&fr.remote_addr, sizeof(fr.remote_addr));
  return;
}

void send_synack(const struct file_request& fr) {
  char buf[1];
  *buf = SYN | ACK;
  sendto(fr.sock_fd, buf, 1, 0,
         (struct sockaddr *)&fr.remote_addr, sizeof(fr.remote_addr));
  return;
}

void send_ackfile(const struct file_request& fr) {
  char buf[5];
  buf[0] = ACK;

  if(fr.file_size == -1) {
    std::cout << "send_ackfile ERROR: Invalid file request passed!\n";
    return;
  }

  // Convert to network byte order.
  file_size = htonl(fr.file_size);

  // Copying the 4-byte integer into the char array.
  std::memcpy(buf + 1, &file_size, 4);

  sendto(fr.sock_fd, buf, 1, 0,
         (struct sockaddr *)&fr.remote_addr, sizeof(fr.remote_addr));
  return;
}

void send_file(const struct file_request& fr) {
  char buf[BUFFSIZE];
  int current_packet = 0;
  int current_packet_network = 0;
  int failures = 0;
  buf[0] = PACK;
  std::pair<char*, int> chunk;
  chunk.first = buf[5];
  chunk.second = -1; // -1 denotes EOF.

  while((chunk = get_chunk(fr.infile, BUFFSIZE - 5)).second != -1) {
    current_packet_network = htonl(current_packet);
    std::memcpy(buf + 1, &current_packet_network, 4);
    send_packet(fr, buf, chunk.second);

    
    // Timeout!!




int size_of_file(const std::string &file_name) {
  std::ifstream infile(file_name, std::ios::in | std::ios::binary);
  if (!infile) {
    return -1;
  }
  infile.ignore(std::numeric_limits<std::streamsize>::max());
  return infile.gcount();
}

void send_yes_file(int sockfd, struct sockaddr_in *remote_addr, int file_size) {
  std::string msg_str = "YES" + std::to_string(file_size);
  send_string(sockfd, remote_addr, msg_str);
}

void send_close(int sockfd, struct sockaddr_in *remote_addr) {
  send_string(sockfd, remote_addr, "CLOSE");
}

void send_file_packet(int sockfd, struct file_request& fr, int packet_num) {
  int packet_size;
  if((packet_num + 1) * BUFFSIZE > fr.file_size) {
    packet_size = fr.file_size - (packet_num * BUFFSIZE);
    if(packet_size < 0) {
      std::cout << "send_file_packet: Invalid packet num!\n";
      return;
    }
  }
  else {
    packet_size = BUFFSIZE;
  }

  std::ifstream infile(fr.file_name, std::ios::binary);

  if(infile.is_open()) {
    std::cout << "Sending packet " << packet_num << " of "
              << fr.file_name << ", " << packet_size << " bytes\n";
    std::unique_ptr<char> buf(new char[packet_size]);
    infile.seekg(packet_num * BUFFSIZE);
    infile.read(buf.get(), packet_size); // NO NULL BYTE AT THE END!! CAREFUL!
    sendto(sockfd, buf.get(), packet_size, 0,
         (struct sockaddr *)&fr.remote_addr, sizeof(fr.remote_addr));
  }

  else {
    std::cout << "Unable to open file.\n";
    send_close(sockfd, &fr.remote_addr);
  }
}


void process_payload(int s, struct sockaddr_in *remote_addr) {
  struct file_request fr;
  char buf[BUFFSIZE];
  unsigned int addrlen = sizeof(*remote_addr);

  for (;;) {
    int recvlen =
        recvfrom(s, buf, BUFFSIZE, 0, (struct sockaddr *)remote_addr, &addrlen);
    std::cout << "Received " << recvlen << " bytes.\n";

    // Handshake.
    if (recvlen == 0) {
      std::cout << "Handshake received.\n";
      fr.remote_addr = *remote_addr;
      send_ack(s, &fr.remote_addr);
    }

    // Get path.
    else if (std::string(buf, 3) == "GET") {
      if (*remote_addr == fr.remote_addr) {
        std::string file_name(buf + 3);
        std::cout << "Got request for file_name " << file_name << "\n";
        int file_size = size_of_file(file_name);
        if (file_size >= 0) {
          std::cout << "File read, size " << file_size << "\n";
          fr.file_name = file_name;
          fr.file_size = file_size;
          send_yes_file(s, &(fr.remote_addr), file_size);
        } else {
          std::cout << "File cannot be read.\n";
          send_close(s, &(fr.remote_addr));
        }
      }
    }

    else if (std::string(buf, 3) == "PAC") {
      if(*remote_addr == fr.remote_addr && !fr.file_name.empty()) {
        std::stringstream ss(buf + 3);
        int packet_num;
        if(ss >> packet_num && packet_num <= fr.file_size / BUFFSIZE) {
          send_file_packet(s, fr, packet_num);
        }
        else {
          send_close(s, &(fr.remote_addr));
        }
      }
    }

    else {
      std::cout << "Got nonsensical packet. Contents:\n" << buf << "\nIgnoring.\n";
    }
        
    std::memset(buf, 0, sizeof(buf));
  }
}

*/

int main(int argv, char **argc) {
  int port;
  struct sockaddr_in my_addr;

  // Remote socket.
  struct sockaddr_in remote_addr;
  int recvlen;

  int sockfd;

  if (argv != 2 || !(std::stringstream(argc[1]) >> port) || port < 0 ||
      port >= 65536) {
    std::cout << "Invalid arguments.\n";
    exit(1);
  }

  // Setting various flags in my_addr.
  // htons = host-to-network short. int -> 16-bit network
  // htonl = host-to-network long. int -> 32-bit network
  std::memset((char *)&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  my_addr.sin_port = htons(port);

  // Open the socket.
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    std::cout << "Unable to open socket. Aborting.\n";
    exit(1);
  }

  // Bind the socket.
  if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
    std::cout << "Unable to bind socket. Aborting.\n";
    exit(1);
  }

  std::cout << "Waiting on port " << ntohs(my_addr.sin_port) << "\n";

  for(;;)
     await_syn(sockfd, &my_addr);

  return 0;
}
