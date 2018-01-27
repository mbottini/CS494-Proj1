#ifndef FILEREQUEST
#define FILEREQUEST
#include <string> 
#include <cstring>     // memcpy
#include <ostream>
#include <fstream>
#include <limits>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
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
    void receive_req();
    bool open_file();
    void send_reqack();
    void send_close();
    friend std::ostream& operator <<(std::ostream& os, const FileRequest& fr);
};

bool is_req(char c);
int get_file_size(std::ifstream& infile);

#endif
