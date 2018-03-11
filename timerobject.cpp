#include "timerobject.h"

TimerObject::TimerObject(char* buf, int buff_size, int sockfd, 
                         struct sockaddr_in dest_addr) {
  std::memcpy(this->buf, buf, buff_size);
  this->buff_size = buff_size;
  this->sockfd = sockfd;
  this->dest_addr = dest_addr;
  time(&(this->timestamp));
  timeout_counter = 0;
}

void TimerObject::send_pack() {
  sendto(this->sockfd, this->buf, this->buff_size, 0, 
         (struct sockaddr*)&this->dest_addr, sizeof(this->dest_addr));
}

int TimerObject::get_timeout() {
  return this->timeout_counter;
}

void TimerObject::inc_timeout() {
  (this->timeout_counter++);
}

time_t TimerObject::get_time() {
  return this->timestamp;
}

void TimerObject::set_time(time_t t) {
  this->timestamp = t;
}
