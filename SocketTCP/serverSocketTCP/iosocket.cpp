#include "iosocket.h"

// using to analyser user command.
void splits_string(const std::string& subject, std::vector<std::string>& container)
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

void send_TCP(user_command& user_command, client_list& client_socket_list, fd_set& master, int& fdmax)
{
    std::vector<std::string> container;
    int socket_for_client;
    client_information client_socket_information;

    const unsigned int bufsize =1024;
    char buffer[bufsize];

    unsigned long total_bytes, byte_left, status, sended_bytes;

    fd_set send_fds;
    FD_ZERO(&send_fds);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;


    /* To send massage to a client. The user's input must be format as:
     *              <massege>/<number soket file decriptor >
     * Example: to send client connected with server on socket which have file decriptor 4 massage: "abcd"
     * the user's input: abcd/4
     *
     */
    while(1)
    {
        std::unique_lock<std::mutex> locker(user_command_muxtex);
        cond.wait(locker);
        printf("=>Sender walked up \n");


        if (!user_command.empty())
        {
            splits_string(user_command.get(), container);
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
                        if(client_socket_list.find(socket_for_client, client_socket_information) < 0)
                        {
                            printf("Don't have this socket in the list \n");
                        }
                        else
                        {
                            memset(&buffer,0,bufsize/sizeof(char));
                            strcpy(buffer, container[0].c_str());
                            total_bytes = container[0].length();

                            byte_left = total_bytes;
                            sended_bytes = 0;
                            int try_times = 0;

                            while(sended_bytes < total_bytes)
                            {
                                send_fds = master;
                                if (select(fdmax+1,nullptr, &send_fds, nullptr, &tv) <0)
                                {
                                    perror("select");
                                    exit(EXIT_FAILURE);
                                };

                                if(FD_ISSET(socket_for_client, &send_fds))
                                {
                                    status = send(socket_for_client, buffer+sended_bytes, byte_left, 0);
                                    if (status == -1UL)
                                    {
                                        printf("Sending failure !!!");
                                        if (try_times ++ < 10)
                                        {
                                            break;
                                        };
                                    }
                                    else
                                    {
                                        sended_bytes += status;
                                        byte_left -= status;
                                    };
                                }
                                else
                                {
                                    printf("Socket is not ready to send data!! \n");
                                    if (try_times ++ < 10)
                                    {
                                        printf("Error on sending message");
                                        break;
                                    };
                                };
                            };
                        };
                    };
                }
                else
                {
                    printf(" Wrong format message or no massge to send !! \n");
                };
            };
            user_command.clear();
        }
        else
        {
            sleep(1);
        };
        printf("=>Sender sleep again. \n");
    };
};
