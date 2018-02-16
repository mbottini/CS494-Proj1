#include "client.h"

// Argv contents:
// 0 : Name of program
// 1 : IP Address (e.g. 192.168.88.254)
// 2 : Port (16-bit integer)
// 3 : File path on the server.
// 4 : Optional path where you want to save the output.
//     Note that if omitted, the program prints to stdout.

int main(int argc, char **argv) {
  // Server socket.
  struct sockaddr_in dest_addr;
  int dest_port;
  std::ostream *os;
  std::ofstream outfile;
  int current_packet = 0;

  int sock_handle;

  if (argc < 4 || argc > 5) {
    std::cerr << "Invalid number of arguments. Exiting.\n";
    return 1;
  }

  char *hostname = argv[1];
  char *port = argv[2];
  int port_num = std::atoi(port);

  if(port_num <= 0 || port_num >= 65536) {
    std::cerr << "Invalid port number. Exiting.\n";
    return 2;
  }

  if (!hostname_to_ip(hostname, port, (struct sockaddr*)&dest_addr)) {
    std::cerr << "Invalid IP address. Exiting.\n";
    return 3;
  }

  if (!(std::stringstream(argv[2]) >> dest_port) || dest_port < 0 ||
      dest_port >= 65536) {
    std::cerr << "Invalid port number. Exiting.\n";
    return 4;
  }

  // Open the socket.
  sock_handle = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_handle < 0) {
    std::cerr << "Unable to open socket. Aborting.\n";
    return 5;
  }

  // Set the timeout.
  struct timeval tv;
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;
  if(setsockopt(sock_handle, SOL_SOCKET, (SO_RCVTIMEO), &tv,
                sizeof(tv)) < 0) {
    std::cerr << "Unable to set options. Aborting\n";
    return 6;
  }

  if(argc == 5) {
    outfile.open(argv[4]);
    if(!outfile) {
      std::cerr << "Unable to open file. Aborting.\n";
      return 7;
    }
    os = &outfile;
  }

  else {
    os = &std::cout;
  }

  std::function<void(void)> send_f;
  std::function<rec_outcome(void)> rec_f;
  rec_outcome result;

  send_f = std::bind(send_syn, sock_handle, &dest_addr);
  rec_f = std::bind(receive_synack, sock_handle, &dest_addr);

  result = try_n_times_ternary(send_f, rec_f, BADTIMEOUT);
  if(result == REC_FAILURE) {
    std::cerr << "Request received failure message.\n";
    return 0;
  }
  else if(result == REC_TIMEOUT) {
    std::cerr << "Request timed out.\n";
    return 0;
  }

  send_f = std::bind(send_req, sock_handle, &dest_addr, argv[3]);
  rec_f = std::bind(receive_reqack, sock_handle, &dest_addr);

  result = try_n_times_ternary(send_f, rec_f, BADTIMEOUT);
  if(result == REC_FAILURE) {
    std::cerr << "Request received failure message.\n";
    return 0;
  }
  else if(result == REC_TIMEOUT) {
    std::cerr << "Request timed out.\n";
    return 0;
  }

  // First pack, and first packet only.
  send_f = std::bind(send_packsyn, sock_handle, &dest_addr, 50);
  rec_f = std::bind(receive_pack, sock_handle, &dest_addr, &current_packet, os);

  result = try_n_times_ternary(send_f, rec_f, BADTIMEOUT);
  if(result == REC_FAILURE) {
    std::cerr << "Request received failure message.\n";
    return 0;
  }
  else if(result == REC_TIMEOUT) {
    std::cerr << "Request timed out.\n";
    return 0;
  }


  send_f = std::bind(send_packack, sock_handle, &dest_addr, &current_packet);

  // Remainder of packets.
  do {
    result = try_n_times_ternary(send_f, rec_f, BADTIMEOUT);
  } while (result == REC_SUCCESS);
    
  return 0;
}

