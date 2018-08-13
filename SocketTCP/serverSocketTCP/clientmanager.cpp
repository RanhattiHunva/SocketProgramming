#include "clientmanager.h"

void client_list::add(const char* inputIP, int inputNum)
{
    std::lock_guard<std::mutex> guard(client_mutext);
    strcpy(client_element.IP_addr,inputIP);
    client_element.num_socket = inputNum;
    client_list.push_back(client_element);
};

int client_list::find(int inputNum, client_information& contain_information)
{
    std::lock_guard<std::mutex> guard(client_mutext);
    for (unsigned long i=0; i< client_list.size(); i++)
    {
        if (client_list[i].num_socket == inputNum)
        {
            contain_information = client_list[i];
            return 0;
        };
    };
    return -1;
};

int client_list::remove(int inputNum)
{
    std::lock_guard<std::mutex> guard(client_mutext);
    for (unsigned long i=0; i< client_list.size(); i++)
    {
        if (client_list[i].num_socket == inputNum)
        {
            client_list.erase(client_list.begin()+static_cast<long>(i));
            return 0;
        };
    };
    return -1;
};

unsigned long client_list::size()
{
    std::lock_guard<std::mutex> guard(client_mutext);
    return client_list.size();
};
