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
    void addElement(const char* inputIP, int inputNum);
    int findElement(int inputNum, client_information& contain_information);
    int removeElement(int inputNum);
    unsigned long getSize();
};

#endif // CLIENTMANAGER_H
