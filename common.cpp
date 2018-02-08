#include "commondefs.h"

bool try_n_times(std::function<void(void)> send_f, 
                 std::function<bool(void)> rec_f, int n) {
  for(int i = 0; i < n; i++) {
    send_f();
    if(rec_f()) {
      return true;
    }
  }
  return false;
}


// Terminates upon receiving a positive value.
// Negative values denote timeout. Positive is a close() packet.
bool try_n_times_ternary(std::function<void(void)> send_f, 
                         std::function<rec_outcome(void)> rec_f, int n) {
  for(int i = 0; i < n; i++) {
    send_f();
    int result = rec_f();
    if(result == REC_SUCCESS) {
      return true;
    }
    if(result == REC_FAILURE) {
      return false;
    }
  }
  return false;
}
