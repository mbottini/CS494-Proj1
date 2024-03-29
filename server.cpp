#include "server.h"
#include <iostream>
#include <sstream>
#include <thread>

void await_syn(int sockfd, struct sockaddr_in *remote_addr) {
  char buf[BUFFSIZE];
  socklen_t addrlen = sizeof(*remote_addr);
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
  seq_subordinate(fr);
  std::cout << "Sending close.\n";
  fr.send_close();
  std::cout << "Exiting thread.\n";
}

// We declare another function because GOTO is very bad, but we need it.
// As a result, we use another function and use `return` as GOTO end.
void seq_subordinate(FileRequest& fr) {

  std::function<void(void)> send_f;
  std::function<rec_outcome(void)> rec_f;
  rec_outcome result;

  send_f = std::bind(&FileRequest::send_synack, &fr);
  rec_f = std::bind(&FileRequest::receive_req, &fr);
  result = try_n_times(send_f, rec_f, BADTIMEOUT);

  if(result == REC_TIMEOUT) {
    std::cout << "Timed out at receive_req\n";
    return;
  }

  if(result == REC_FAILURE) {
    std::cout << "Received garbage at receive_req\n";
    return;
  }

  if(!fr.open_file()) {
    std::cout << "Unable to open file.\n";
    return;
  }

  send_f = std::bind(&FileRequest::send_reqack, &fr);
  rec_f = std::bind(&FileRequest::receive_packsyn, &fr);
  result = try_n_times(send_f, rec_f, BADTIMEOUT);
  
  if(result == REC_TIMEOUT) {
    std::cout << "Timed out at receive_packsyn\n";
    return;
  }

  if(result == REC_FAILURE) {
    std::cout << "Received garbage at receive_packsyn\n";
    return;
  }

  fr.send_packs();
  return;
}
  

bool is_syn(char c) {
    return c == SYN;
}

int main(int argv, char **argc) {
  int port;
  struct sockaddr_in my_addr;
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
