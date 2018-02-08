#include "commondefs.h"

bool try_n_times(std::function<void(void)> send_f, std::function<bool(void)> rec_f, int n) {
  for(int i = 0; i < n; i++) {
    send_f();
    if(rec_f()) {
      return true;
    }
  }
  return false;
}
