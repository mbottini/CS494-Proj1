#ifndef COMMONDEFS_H
#define COMMONDEFS_H

#include <functional>

#define BUFFSIZE 2048
#define TIMEOUT 1
#define BADTIMEOUT 5

#define CLOSE   0b0 // Close.
#define SYN     0b1 // Handshake.
#define ACK    0b10 // Acknowledge.
#define REQ   0b100 // File request.
#define PACK 0b1000 // File packet.

enum rec_outcome {
  REC_TIMEOUT,
  REC_SUCCESS,
  REC_FAILURE
};

bool try_n_times(std::function<void(void)> send_f, 
                 std::function<bool(void)> rec_f, int n);

bool try_n_times_ternary(std::function<void(void)> send_f, 
                         std::function<rec_outcome(void)> rec_f, int n);

#endif
