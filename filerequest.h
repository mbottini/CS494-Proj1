#ifndef FILEREQUEST
#define FILEREQUEST
#include <string> 
#include <cstring>     // memcpy
#include <ostream>
#include <fstream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "commondefs.h"

#define TIMEOUT 1
#define BADTIMEOUT 5
#define BUFFSIZE 2048

class FileRequest {
  private:
    int sockfd;
    struct sockaddr_in my_addr;
    struct sockaddr_in remote_addr;
    std::string filename;
    std::ifstream infile;
    int file_size;
    int current_packet;

    void set_null();
  public:
    FileRequest(struct sockaddr_in remote_addr);
    ~FileRequest();
    void send_synack();
    friend std::ostream& operator <<(std::ostream& os, const FileRequest& fr);
};


#endif
