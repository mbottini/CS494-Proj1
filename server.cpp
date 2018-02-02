#include "server.h"
#include <iostream>
#include <sstream>
#include <thread>

/* TODO: Require a debug flag to print debug messages.
 *       Each send-receive pair must be enclosed inside a while(BADTIMEOUT)
 *       loop, with a send_close() upon failure.
 *       Something as follows:
 *       void protocol() {
 *           int counter = 0;
 *           bool result = false;
 *           do {
 *               fr.do_thing();
 *               result = fr.receive_thing();
 *               counter++;
 *           } while(!result && counter < BADTIMEOUT);
 *           if(counter >=BADTIMEOUT) {
 *               return;
 *           }
 *           counter = 0;
 *           
 *           We can then encapsulate this inside another function.
 *           bool tryNtimes(std::function<void> send_func, std::function<void> rec_func)
 *
 *           Same idea. Try it, and if it returns true, we're good. If not, welp, return false and return.
 *           The end of the calling function then sends the close.
 *           It's basically a goto, but we're pretending it's not because it's a function call. lololol
 */


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

  std::function<void(void)> f1;
  std::function<bool(void)> f2;

  f1 = std::bind(&FileRequest::send_synack, &fr);
  f2 = std::bind(&FileRequest::receive_req, &fr);
  if(!try_n_times(f1, f2, BADTIMEOUT)) {
    return;
  }

  if(!fr.open_file()) {
    return;
  }

  f1 = std::bind(&FileRequest::send_reqack, &fr);
  f2 = std::bind(&FileRequest::receive_packsyn, &fr);
  if(!try_n_times(f1, f2, BADTIMEOUT)) {
    return;
  }

  fr.send_packs();
  return;
}
  

bool is_syn(char c) {
    return c == SYN;
}

bool try_n_times(std::function<void(void)> send_f, std::function<bool(void)> rec_f, int n) {
  for(int i = 0; i < n; i++) {
    send_f();
    if(rec_f()) {
      return true;
    }
  }
  return false;
}

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
