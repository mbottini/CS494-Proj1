#ifndef FILEREQUEST
#define FILEREQUEST
#include <string> 
#include <cstring>     // memcpy
#include <ostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <memory>      // std::unique_ptr
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>

#include "commondefs.h"

class FileRequest {
  private:
    int sockfd;
    struct sockaddr_in my_addr;
    struct sockaddr_in remote_addr;
    std::string filename;
    std::ifstream infile;
    int file_size;
    int packet_size;
    int current_packet;

    void set_null();

  public:
    FileRequest(struct sockaddr_in remote_addr);
    ~FileRequest();
    void send_synack();
    rec_outcome receive_req();
    bool open_file();
    void send_reqack();
    rec_outcome receive_packsyn();
    void send_packs();
    int receive_packack();
    void send_close();
    friend std::ostream& operator <<(std::ostream& os, const FileRequest& fr);
};

bool is_req(char c);
bool is_packsyn(char c);
bool is_packack(char c);
int get_file_size(std::ifstream& infile);
int copy_chunk(char* buf, std::ifstream& infile, int size);

#endif
