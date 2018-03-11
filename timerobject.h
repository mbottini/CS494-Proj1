#ifndef TIMEROBJECT_H
#define TIMEROBJECT_H
#include "commondefs.h"
#include <memory>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <time.h>
#include <cstring>

class TimerObject {
  private:
    time_t timestamp;
    char buf[BUFFSIZE];
    int buff_size;
    int sockfd;
    struct sockaddr_in dest_addr;
    int timeout_counter;

  public:
    TimerObject(char* buf, int buff_size, int sockfd, struct sockaddr_in dest_addr);
    void send_pack();
    int get_timeout();
    void inc_timeout();
    time_t get_time();
    void set_time(time_t t);
};
#endif
