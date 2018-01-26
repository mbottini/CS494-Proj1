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

std::ostream& operator <<(std::ostream& os, const FileRequest& fr) {
  os << "Socket handle: " << fr.sockfd << "\n";
  os << "Host port: " << fr.my_addr.sin_port << "\n";
  os << "Remote address: " << ntohl(fr.remote_addr.sin_addr.s_addr) << "\n";
  return os;
}

