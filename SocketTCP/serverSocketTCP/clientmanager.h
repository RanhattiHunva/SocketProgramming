#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <arpa/inet.h>
#include <vector>
#include <mutex>
#include <string.h>

struct  client_information
{
    char IP_addr[INET6_ADDRSTRLEN];
    int num_socket;
    char user_name[1024];
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
    int get_by_order(unsigned long order,  client_information& contain_information);
    int remove_by_order(unsigned long order);
    int set_user_name(int inputNum, const char* user_name);
    int get_by_user_name(const char* user_name);
};

#endif // CLIENTMANAGER_H
