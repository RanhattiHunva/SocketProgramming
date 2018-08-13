#include "iosocket.h"

// using to analyser user command.
void splitString(const std::string& subject, std::vector<std::string>& container)
{
    container.clear();
    size_t len = subject.length()+ 1;
    char* s = new char[ len ];
    memset(s, 0, len*sizeof(char));
    memcpy(s, subject.c_str(), (len - 1)*sizeof(char));
    for (char *p = strtok(s, "/"); p != nullptr; p = strtok(nullptr, "/"))
    {
        container.push_back(p);
    }
    delete[] s;
}

void sendTCP(user_command& userCommand, client_list& client_socket_list, fd_set& master, int& fdmax)
{
    std::vector<std::string> container;
    int socket_for_client;
    client_information client_socket_information;

    const unsigned int bufsize =1024;
    char buffer[bufsize];

    unsigned long validElements, byteLeft, status, totalBytes;

    fd_set send_fds;
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 50000;


    /* To send massage to a client. The user's input must be format as:
     *              <massege>/ <number soket file decriptor >
     * Example: to send client with connect with server on socket which have file decriptor 4 massage: "abcd"
     * the user's input: abcd/4
     *
     */
    while(1)
    {
        send_fds = master;
        std::unique_lock<std::mutex> locker(user_command_muxtex);
        cond.wait(locker);
        if (!userCommand.empty())
        {
            splitString(userCommand.get(), container);
            locker.unlock();
            
            if (!container[0].compare("#"))
            {
                break;
            }
            else
            {
                if(container.size() == 2)
                {
                    bool isNumber = true;
                    try
                    {
                        socket_for_client = stoi(container[1]);
                    }
                    catch(...)
                    {
                        printf(" The folowing text is no a number !! \n");
                        isNumber = false;
                    };

                    if (isNumber)
                    {
                        if(client_socket_list.findElement(socket_for_client, client_socket_information) < 0)
                        {
                            printf("Don't hove this socket in the list");
                        }
                        else
                        {
                            memset(&buffer,0,bufsize/sizeof(char));
                            strcpy(buffer, container[0].c_str());
                            validElements = container[0].length();

                            byteLeft = validElements;
                            totalBytes = 0;

                            while(totalBytes < validElements)
                            {
                                if (select(fdmax+1,nullptr, &send_fds, nullptr, &tv) <0)
                                {
                                    perror("select");
                                    exit(EXIT_FAILURE);
                                };

                                if(FD_ISSET(socket_for_client, &send_fds)){
                                    status = send(socket_for_client, buffer+totalBytes, byteLeft, 0);
                                    if (status == -1UL)
                                    {
                                        printf("Sending failure !!!");
                                        throw "Error sending";
                                    }
                                    else
                                    {
                                        totalBytes += status;
                                        byteLeft -= status;
                                    };
                                }else{
                                    printf("Socket is not ready to send data!!");
                                };
                            };
                        };
                    };
                }
                else
                {
                    printf(" Wrong format message or no massge to send !!");
                };
            };
            userCommand.clear();
        }else{
            sleep(1);
        };
    };
};
