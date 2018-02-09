#include "filerequest.h"
#include <iostream>

FileRequest::FileRequest(struct sockaddr_in remote_addr) {
  this->sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if(this->sockfd < 0) {
    set_null();
    return;
  }

  // Setting up the sockaddr_in for the object.
  this->my_addr.sin_family = AF_INET;
  this->my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  this->my_addr.sin_port = htonl(0);

  // Binding the socket to my_addr.

  if(bind(this->sockfd, (struct sockaddr*)&this->my_addr, sizeof(my_addr)) < 0) {
    set_null();
    return;
  }

  struct timeval tv;
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;

  if(setsockopt(sockfd, SOL_SOCKET, (SO_RCVTIMEO), &tv,
                sizeof(tv)) < 0) {
    std::cout << "Unable to set options.\n";
    set_null();
    return;
  }


  // We're now bound, and we can copy remote_addr to this->remote_addr.

  this->remote_addr = remote_addr;

  // Other members are set to invalid values for the future.
  this->filename = "";
  this->file_size = -1;
  this->packet_size = -1;
  this->current_packet = -1;
  return;
}

FileRequest::~FileRequest() {
  // We only care about the socket getting released. Everything else gets closed
  // automatically.
  if(this->sockfd >= 0) {
    close(this->sockfd);
  }
  // TODO: Resources say that it's a bad idea not to check the return value of
  // close. Why? Does it matter for this application?
}
  

void FileRequest::set_null() {
  char buf[sizeof(struct sockaddr_in)] = {0};
  std::memcpy(&(this->my_addr), buf, sizeof(struct sockaddr_in));
  std::memcpy(&(this->remote_addr), buf, sizeof(struct sockaddr_in));
  this->filename = "";
  this->file_size = -1;
  this->packet_size = -1;
  this->current_packet = -1;
  return;
}

void FileRequest::send_synack() {
  char buf; // Only one element!
  buf = SYN | ACK;
  sendto(this->sockfd, &buf, 1, 0, 
         (struct sockaddr*)&this->remote_addr, sizeof(this->remote_addr));
  return;
}

bool FileRequest::receive_req() {
  char buf[BUFFSIZE];
  socklen_t addrlen = sizeof(this->remote_addr);
  int recvlen = recvfrom(this->sockfd, buf, BUFFSIZE, 0,
                         (struct sockaddr*)&(this->remote_addr), &addrlen);
  if(recvlen > 1 && is_req(*buf)) {
    filename = std::string(buf + 1, recvlen - 1);
  }
  /* We're going to make this function produce a bool,, and the calling function
   * will instead send the close. 
  else {
    send_close();
  }
  */
  return recvlen > 0;
}

bool FileRequest::open_file() {
  this->infile.open(this->filename);
  if(this->infile.is_open()) {
    this->file_size = get_file_size(infile);
    this->current_packet = 0;
  }
  return this->infile.is_open();
}

void FileRequest::send_reqack() {
  if(this->file_size < 0 || !infile.is_open()) {
    send_close();
  }

  int file_size_network = htonl(this->file_size);

  char buf[5];
  *buf = REQ | ACK;
  std::memcpy(buf + 1, &file_size_network, 4);
  
  sendto(this->sockfd, &buf, 5, 0, 
         (struct sockaddr*)&this->remote_addr, sizeof(this->remote_addr));
}

bool FileRequest::receive_packsyn() {
  char buf[BUFFSIZE];
  socklen_t addrlen = sizeof(this->remote_addr);
  int packet_size = 0;
  bool valid_packet = false;
  int recvlen = recvfrom(this->sockfd, buf, BUFFSIZE, 0,
                         (struct sockaddr*)&(this->remote_addr), &addrlen);
  if(recvlen == 5 && is_packsyn(*buf)) {
    std::cout << "PACKSYN received.\n";
    memcpy(&packet_size, buf + 1, 4);
    packet_size = ntohl(packet_size);
    if(packet_size > BUFFSIZE) {
      packet_size = BUFFSIZE;
    }
    this->packet_size = packet_size;
    valid_packet = true;
  }

  /* See above. We're going to make this return a bool, so the calling function
   * will have to send the close. 
  else {
    send_close();
  }
  */
  return valid_packet;
}

// Unfortunately, we can't do try_n_times with this because we're passing
// an arg. We could write it that way, but it would involve declaring
// an array for every packet. That would be silly.
void FileRequest::send_packs() {
  std::unique_ptr<char[]> buf(new char[packet_size]);
  *buf.get() = PACK;
  int actual_size = 0;
  int packack_value;
  int timeout_count;
  while((actual_size = copy_chunk(buf.get() + 5, infile, packet_size - 5)) > 0) {
    actual_size += 5;
    timeout_count = 0;

    int current_packet_network = htonl(this->current_packet);
    std::memcpy(buf.get() + 1, &current_packet_network, 4);

    do {
      // We've tried to send the packet BADTIMEOUT times, and failed each time.
      if(timeout_count >= BADTIMEOUT) {
        return;
      }
      sendto(this->sockfd, buf.get(), actual_size, 0, 
             (struct sockaddr*)&this->remote_addr, sizeof(this->remote_addr));
      packack_value = receive_packack();
    } while (packack_value != this->current_packet);
    this->current_packet++;
  }
  return;
}

int FileRequest::receive_packack() {
  char buf[5];
  socklen_t addrlen = sizeof(this->remote_addr);
  int packet_number = -1;
  int recvlen = recvfrom(this->sockfd, buf, 5, 0,
                         (struct sockaddr*)&(this->remote_addr), &addrlen);
  if(recvlen == 5 && is_packack(*buf)) {
    memcpy(&packet_number, buf + 1, 4);
    packet_number = ntohl(packet_number);
  }
  return packet_number;
}


void FileRequest::send_close() {
  char buf = CLOSE;
  sendto(this->sockfd, &buf, 1, 0,
         (struct sockaddr*)&this->remote_addr, sizeof(this->remote_addr));
}

std::ostream& operator <<(std::ostream& os, const FileRequest& fr) {
  os << "Socket handle: " << fr.sockfd << "\n";
  os << "Host port: " << fr.my_addr.sin_port << "\n";
  os << "Remote address: " << ip_to_string(ntohl(fr.remote_addr.sin_addr.s_addr)) 
                           << "\n";
  return os;
}

bool is_req(char c) {
  return c == REQ;
}

bool is_packsyn(char c) {
  return c == (PACK | SYN);
}

bool is_packack(char c) {
  return c == (PACK | ACK);
}

int get_file_size(std::ifstream& infile) {
  if(!infile.is_open()) {
    return -1;
  }

  infile.ignore(std::numeric_limits<std::streamsize>::max());
  int file_size = infile.gcount();
  infile.seekg(0);
  return file_size;
}

int copy_chunk(char *buf, std::ifstream& infile, int size) {
  if(!infile || !infile.is_open()) {
    return 0;
  }

  infile.read(buf, size);
  return infile.gcount();
}
