#ifndef THREADFUNCTION_H
#define THREADFUNCTION_H

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <vector>
#include <fcntl.h>

#include "clientmanager.h"
#include "usercommand.h"

extern std::mutex user_command_muxtex;
extern std::condition_variable cond;

void splits_string(const std::string& subject, std::vector<std::string>& container);

void send_TCP(user_command& user_command, client_list& client_socket_list,fd_set& master, int& fdmax);

#endif // THREADFUNCTION_H
