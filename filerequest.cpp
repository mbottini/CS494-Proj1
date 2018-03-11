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

rec_outcome FileRequest::receive_req() {
  char buf[BUFFSIZE];
  socklen_t addrlen = sizeof(this->remote_addr);
  int recvlen = recvfrom(this->sockfd, buf, BUFFSIZE, 0,
                         (struct sockaddr*)&(this->remote_addr), &addrlen);
  if(recvlen > 1 && is_req(*buf)) {
    filename = std::string(buf + 1, recvlen - 1);
    return REC_SUCCESS;
  }
  else if(recvlen == -1) {
    return REC_TIMEOUT;
  }
  return REC_FAILURE;
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

rec_outcome FileRequest::receive_packsyn() {
  char buf[BUFFSIZE];
  socklen_t addrlen = sizeof(this->remote_addr);
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
    return REC_SUCCESS;
  }
  else if(recvlen == -1) {
    return REC_TIMEOUT;
  }

  return REC_FAILURE;
}

rec_outcome FileRequest::send_packs() {
  unsigned int window_size = WINDOW_START;
  std::unordered_map<int, TimerObject> timer_map;
  int current_packet = 0;
  int current_packet_htonl = 0;
  char current_buf[BUFFSIZE];
  int actual_size;
  bool eof = false;
  time_t now;
  bool decrease_window = false;

  // Socket reception variables.
  int recvlen = -1;
  char buf[BUFFSIZE];
  socklen_t addrlen = sizeof(this->remote_addr);
  int packet_number;

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000; // 0.1 seconds
  // Note that we don't care about resetting the options, as this is the last
  // step in the file exchange.
  if(setsockopt(sockfd, SOL_SOCKET, (SO_RCVTIMEO), &tv,
                sizeof(tv)) < 0) {
    std::cout << "Unable to set timeout options.\n";
    return REC_FAILURE;
  }

  while(1) {
    if(eof && timer_map.empty()) {
      break;
    }
    // Grab another chunk and create another timer object in the map.
    if(!eof && timer_map.size() < window_size) {
      current_buf[0] = PACK;
      current_packet_htonl = htonl(current_packet);
      std::memcpy(buf + 1, &current_packet_htonl, 4);
      actual_size = copy_chunk(current_buf + 5, this->infile, BUFFSIZE - 5);
      if(actual_size <= 0) {
        eof = true;
      }
      else {
        actual_size += 5;
        timer_map.emplace(current_packet, 
                          TimerObject(current_buf, actual_size, this->sockfd,
                                      this->remote_addr));
      }
    }
  
    // Spinning, spinning, spinning... There are async ways to do this, but the
    // documentation is actively hostile to me applying it to this task, and
    // I'm doubtful that it'll aid in efficiency anyway.
    decrease_window = false;
    time(&now);
    for(auto it = timer_map.begin(); it != timer_map.end(); ++it) {
      if(difftime(now, it->second.get_time()) >= TIMEOUT) {
        it->second.send_pack();
        it->second.set_time(now);
        it->second.inc_timeout();
        if(!decrease_window) {
          decrease_window = true;
          window_size /= 2;
          if(window_size == 0) {
            window_size++;
          }
        }
        if(it->second.get_timeout() > BADTIMEOUT) {
          return REC_FAILURE;
        }
      }
    }

    // Receive a packet. If it times out, return to beginning. Otherwise,
    // we process the packet and remove the corresponding entry from the map. 
    recvlen = recvfrom(this->sockfd, buf, BUFFSIZE, 0,
                         (struct sockaddr*)&(this->remote_addr), &addrlen);
    if(recvlen == 5 && is_packack(*buf)) {
      std::memcpy(&packet_number, buf + 1, 4);
      packet_number = htonl(packet_number);
      if(timer_map.find(packet_number) != timer_map.end()) {
        timer_map.erase(packet_number);
      }
    }
  }
  
  return REC_SUCCESS;
}

void FileRequest::send_pack(char* buf, int actual_size) {
  sendto(this->sockfd, buf, actual_size, 0, 
         (struct sockaddr*)&this->remote_addr, sizeof(this->remote_addr));
}

rec_outcome FileRequest::receive_packack() {
  char buf[5];
  socklen_t addrlen = sizeof(this->remote_addr);
  int packet_number = -1;
  int recvlen = recvfrom(this->sockfd, buf, 5, 0,
                         (struct sockaddr*)&(this->remote_addr), &addrlen);
  if(recvlen == 5 && is_packack(*buf)) {
    memcpy(&packet_number, buf + 1, 4);
    packet_number = ntohl(packet_number);
    if(packet_number == this->current_packet) {
      return REC_SUCCESS;
    }
    else {
      std::cout << "Wrong packet. Packet received was " << packet_number <<
              "\n";
    }
  }
  if(recvlen == -1) {
    return REC_TIMEOUT;
  }

  return REC_FAILURE;
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


