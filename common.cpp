#include "commondefs.h"
#include <iostream>

// Terminates upon receiving a positive value.
// Negative values denote timeout. Positive is a close() packet.
rec_outcome try_n_times_ternary(std::function<void(void)> send_f, 
                         std::function<rec_outcome(void)> rec_f, int n) {
  for(int i = 0; i < n; i++) {
    send_f();
    int result = rec_f();
    if(result == REC_SUCCESS) {
      return REC_SUCCESS;
    }
    if(result == REC_FAILURE) {
      return REC_FAILURE;
    }
  }
  return REC_TIMEOUT;
}

std::string ip_to_string(int ip) {
  std::stringstream ss;
  int mask = 0xFF;
  for(int i = 3; i >= 0; i--) {
    ss << ((ip >> (8 * i)) & mask);
    ss << ".";
  }

  return ss.str().substr(0, ss.str().length() - 1);
}

