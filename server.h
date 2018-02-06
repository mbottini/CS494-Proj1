#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <memory>
#include "filerequest.h"
#include "commondefs.h"
#include <functional>

bool operator ==(const struct sockaddr_in &addr1, const struct sockaddr_in &addr);

// Awaits handshakes and opens threads for requests.
void await_syn(int sockfd, struct sockaddr_in *remote_addr);

bool is_syn(char c);

// Upon handshake, await_syn opens a subordinate thread. Executes the
// protocol.
void subordinate_thread(struct sockaddr_in current_addr);
void seq_subordinate(FileRequest& fr);
bool try_n_times(std::function<void(void)> send_f, std::function<bool(void)> rec_f, int n);

#endif





