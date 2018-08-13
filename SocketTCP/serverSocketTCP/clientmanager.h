#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <arpa/inet.h>
#include <vector>
#include <mutex>

struct  client_information
{
    char IP_addr[INET6_ADDRSTRLEN];
    int num_socket;
};

class client_list{
    std::vector <client_information> client_list;
    client_information client_element;
    std::mutex client_mutext;
public:
    void add(const char* inputIP, int inputNum);
    int find(int inputNum, client_information& contain_information);
    int remove(int inputNum);
    unsigned long size();
    client_information get_by_oder(int order);
};

#endif // CLIENTMANAGER_H
