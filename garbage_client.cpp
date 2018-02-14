#include "client.h"
#include <thread>
#include <future>
#include <string>
#include <unordered_map>
#include <vector>

// Argv contents:
// 0 : Name of program
// 1 : IP Address (e.g. 192.168.88.254)
// 2 : Port (16-bit integer)
// 3 : File path on the server.
// 4 : Optional path where you want to save the output.
//     Note that if omitted,the program prints to stdout.

int create_socket(int timeout) {
  // Open the socket.
  int sock_handle = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_handle < 0) {
    std::cerr << "Unable to open socket. Aborting.\n";
    return -1;
  }

  // Bind the socket.
  struct sockaddr_in my_addr;
  std::memset((char *)&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  my_addr.sin_port = 0;

  if (bind(sock_handle, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
    std::cerr << "Unable to bind socket. Aborting.\n";
    return -1;
  }

  // Set the timeout.
  struct timeval tv;
  tv.tv_sec = timeout;
  tv.tv_usec = 0;
  if(setsockopt(sock_handle, SOL_SOCKET, (SO_RCVTIMEO), &tv,
                sizeof(tv)) < 0) {
    std::cerr << "Unable to set options. Aborting\n";
    return -1;
  }

  return sock_handle;
}

void send_shitty_syn(int sockfd, struct sockaddr_in *dest_addr) {
  char buf = 0;
  sendto(sockfd, &buf, 1, 0, (struct sockaddr *)dest_addr,
         sizeof(*dest_addr));
}

rec_outcome receive_close(int sock_handle, struct sockaddr_in *dest_addr) {
  socklen_t addrlen = sizeof(*dest_addr);
  char buf[BUFFSIZE];
  int recvlen = recvfrom(sock_handle, buf, BUFFSIZE, 0, (struct sockaddr *)dest_addr,
                         &addrlen);
  if (recvlen == 1 && is_close(*buf)) {
    return REC_SUCCESS;
  } 
  if(recvlen == -1) {
    return REC_TIMEOUT;
  }
  else {
    return REC_FAILURE;
  }
}

bool test_bad_handshake(int sockfd, struct sockaddr_in dest_addr) {
  std::cerr << "bad_handshake: " << &dest_addr << "\n";

  send_shitty_syn(sockfd, &dest_addr);
  return receive_synack(sockfd, &dest_addr) == REC_TIMEOUT;
}

bool test_no_response_handshake(int sockfd, struct sockaddr_in dest_addr) {
  send_syn(sockfd, &dest_addr);
  rec_outcome result;

  for(int i = 0; i < BADTIMEOUT; i++) {
    result = receive_synack(sockfd, &dest_addr);
    if(result == REC_SUCCESS) {
      std::cerr << "no_response_handshake: Retransmit #" << i << "\n";
    }
    else {
      return false;
    }
  }

  return receive_close(sockfd, &dest_addr);
}

bool test_bad_req(int sockfd, struct sockaddr_in dest_addr) {
  send_syn(sockfd, &dest_addr);
  rec_outcome result = receive_synack(sockfd, &dest_addr);
  if(result != REC_SUCCESS) {
    return false;
  }

  send_shitty_syn(sockfd, &dest_addr);
  return receive_close(sockfd, &dest_addr);
}

bool test_bad_filename(int sockfd, struct sockaddr_in dest_addr) {
  send_syn(sockfd, &dest_addr);
  rec_outcome result = receive_synack(sockfd, &dest_addr);
  if(result != REC_SUCCESS) {
    return false;
  }

  send_req(sockfd, &dest_addr, "ajhksdgbaj");
  return receive_close(sockfd, &dest_addr);
}

bool test_no_response_filename(int sockfd, struct sockaddr_in dest_addr) {
  send_syn(sockfd, &dest_addr);
  rec_outcome result = receive_synack(sockfd, &dest_addr);
  if(result != REC_SUCCESS) {
    return false;
  }

  send_req(sockfd, &dest_addr, "commondefs.h");

  for(int i = 0; i < BADTIMEOUT; i++) {
    result = receive_reqack(sockfd, &dest_addr);
    if(result == REC_SUCCESS) {
      std::cerr << "no_response_filename: Retransmit #" << i << "\n";
    }
    else {
      return false;
    }
  }

  return receive_close(sockfd, &dest_addr) == REC_SUCCESS;
}

bool test_bad_packsyn1(int sockfd, struct sockaddr_in dest_addr) {
  send_syn(sockfd, &dest_addr);
  rec_outcome result = receive_synack(sockfd, &dest_addr);
  if(result != REC_SUCCESS) {
    return false;
  }

  send_req(sockfd, &dest_addr, "commondefs.h");

  result = receive_reqack(sockfd, &dest_addr);
  if(result != REC_SUCCESS) {
    return false;
  }

  send_shitty_syn(sockfd, &dest_addr);
  return receive_close(sockfd, &dest_addr) == REC_SUCCESS;
}

bool test_bad_packsyn2(int sockfd, struct sockaddr_in dest_addr) {
  send_syn(sockfd, &dest_addr);
  rec_outcome result = receive_synack(sockfd, &dest_addr);
  if(result != REC_SUCCESS) {
    return false;
  }

  send_req(sockfd, &dest_addr, "commondefs.h");

  result = receive_reqack(sockfd, &dest_addr);
  if(result != REC_SUCCESS) {
    return false;
  }

  send_packsyn(sockfd, &dest_addr, 3);
  return receive_close(sockfd, &dest_addr) == REC_SUCCESS;
}

bool test_no_response_packsyn(int sockfd, struct sockaddr_in dest_addr) {
  int current_packet = 0;
  send_syn(sockfd, &dest_addr);
  rec_outcome result = receive_synack(sockfd, &dest_addr);
  if(result != REC_SUCCESS) {
    std::cout << "receive_synack: " << result << "\n";
    return false;
  }

  send_req(sockfd, &dest_addr, "commondefs.h");

  result = receive_reqack(sockfd, &dest_addr);
  if(result != REC_SUCCESS) {
    std::cout << "receive_reqack: " << result << "\n";
    return false;
  }

  send_packsyn(sockfd, &dest_addr, 500);

  for(int i = 0; i < BADTIMEOUT; i++) {
    result = receive_pack(sockfd, &dest_addr, &current_packet, &std::cerr);
    current_packet = 0;
    if(result == REC_SUCCESS) {
      std::cerr << "no_response_packsyn: Retransmit #" << i << "\n";
    }
    else {
      std::cout << "receive_pack: " << result << "\n";
      return false;
    }
  }

  return receive_close(sockfd, &dest_addr) == REC_SUCCESS;
}

bool test_retransmit_packs(int sockfd, struct sockaddr_in dest_addr, 
                int times_ignore, std::ostream *os) {
  int current_packet = 0;
  int previous_packet = 0;
  send_syn(sockfd, &dest_addr);
  rec_outcome result = receive_synack(sockfd, &dest_addr);
  if(result != REC_SUCCESS) {
    return false;
  }

  send_req(sockfd, &dest_addr, "commondefs.h");

  result = receive_reqack(sockfd, &dest_addr);
  if(result != REC_SUCCESS) {
    return false;
  }

  send_packsyn(sockfd, &dest_addr, 500);

  for(int i = 0; i < times_ignore; i++) {
    result = receive_pack(sockfd, &dest_addr, &current_packet, &std::cerr);
    current_packet = 0;
    if(result == REC_SUCCESS) {
      *os       << "test_retransmit_packs, packet " << current_packet 
                << ". Ignore #" << i << "\n";
    }
    else {
      *os       << "Received something else " << result << "\n";
      return false;
    }
  }

  result = receive_pack(sockfd, &dest_addr, &current_packet, &std::cerr);
  if(result == REC_SUCCESS) {
    *os       << "test_retransmit_packs, packet " << current_packet 
              << ". Responding.\n";
  }
  else {
    return false;
  }

  while(result == REC_SUCCESS) {
    previous_packet = current_packet;
    send_packack(sockfd, &dest_addr, &current_packet);
    for(int i = 0; i < times_ignore; i++) {
      current_packet = previous_packet;
      result = receive_pack(sockfd, &dest_addr, &current_packet, os);
      if(result == REC_SUCCESS) {
        *os       << "test_retransmit_packs, packet " << current_packet
                  << ". Ignore #" << i << "\n";
      }
      else {
        return true;
      }
    }
    current_packet = previous_packet;
    result = receive_pack(sockfd, &dest_addr, &current_packet, &*os);
    if(result == REC_SUCCESS) {
      *os       << "test_retransmit_packs, packet " << current_packet
                << ". Responding.\n";
    }
    else {
      return false;
    }
  }
  return false;
}

int main(int argc, char **argv) {
  struct sockaddr_in dest_addr;
  std::unordered_map<std::string, std::future<bool>> async_map;

  if (argc < 4 || argc > 5) {
    std::cerr << "Invalid number of arguments. Exiting.\n";
    exit(1);
  }

  if (!hostname_to_ip(argv[1], argv[2], (struct sockaddr*)&dest_addr)) {
    std::cerr << "Invalid IP address. Exiting.\n";
    exit(2);
  }

  async_map.emplace("bad handshake", 
                     std::async(test_bad_handshake, create_socket(1), 
                     dest_addr));
  async_map.emplace("no response after handshake",
                     std::async(std::launch::async, test_no_response_handshake, create_socket(10),
                     dest_addr)); 
  async_map.emplace("bad req",
                     std::async(std::launch::async, test_bad_req, create_socket(1), 
                     dest_addr)); 
  async_map.emplace("bad filename",
                     std::async(std::launch::async, test_bad_filename, create_socket(1), 
                     dest_addr)); 
  async_map.emplace("no response after filename",
                     std::async(std::launch::async, test_no_response_filename, create_socket(10),
                     dest_addr));
  async_map.emplace("bad packsyn packet",
                     std::async(std::launch::async, test_bad_packsyn1, create_socket(1),
                     dest_addr));
  async_map.emplace("bad packsyn size",
                     std::async(std::launch::async, test_bad_packsyn2, create_socket(1),
                     dest_addr));
  async_map.emplace("no response after packsyn",
                     std::async(std::launch::async, test_no_response_packsyn, create_socket(10),
                     dest_addr));
  async_map.emplace("retransmit_packs",
                     std::async(std::launch::async, test_retransmit_packs, create_socket(10),
                     dest_addr, 2, &std::cerr));
  
  for(auto it = async_map.begin(); it != async_map.end(); ++it) {
    std::cout << it->first << ": "
              << (it->second.get() ? "passed" : "failed") << "\n";
  }

  return 0;
}
