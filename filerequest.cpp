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

void FileRequest::receive_req() {
  char buf[BUFFSIZE];
  unsigned int addrlen = sizeof(this->remote_addr);
  int recvlen = recvfrom(this->sockfd, buf, BUFFSIZE, 0,
                         (struct sockaddr*)&(this->remote_addr), &addrlen);
  if(recvlen > 1 && is_req(*buf)) {
    filename = buf + 1;
  }
  else {
    send_close();
  }
  return;
}

void FileRequest::open_file() {
  this->infile.open(this->filename);
  if(this->infile.is_open()) {
    this->file_size = get_file_size(infile);
  }
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

void FileRequest::receive_packsyn() {
  char buf[BUFFSIZE];
  unsigned int addrlen = sizeof(this->remote_addr);
  int packet_size = 0;
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
  }
  else {
    send_close();
  }
  return;
}


void FileRequest::send_close() {
  char buf = CLOSE;
  sendto(this->sockfd, &buf, 1, 0,
         (struct sockaddr*)&this->remote_addr, sizeof(this->remote_addr));
}

std::ostream& operator <<(std::ostream& os, const FileRequest& fr) {
  os << "Socket handle: " << fr.sockfd << "\n";
  os << "Host port: " << fr.my_addr.sin_port << "\n";
  os << "Remote address: " << ntohl(fr.remote_addr.sin_addr.s_addr) << "\n";
  return os;
}

bool is_req(char c) {
  return c == REQ;
}

bool is_packsyn(char c) {
  return c == (PACK | SYN);
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

int get_chunk(std::ifstream& infile, char *buf, int size) {
  if(!infile || !infile.is_open()) {
    return 0;
  }

  infile.read(buf, size);
  return infile.gcount();
}
