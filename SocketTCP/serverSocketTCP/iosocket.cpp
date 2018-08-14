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

void send_TCP(user_command& user_command, client_list& client_socket_list, fd_set& master, int& fdmax, std::vector<int>& input_fds)
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

        if (!user_command.empty())
        {
            splits_string(user_command.get(), container);
            locker.unlock();
            if ((!container[0].compare("#")) && (container.size() == 1))
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
                        printf("=>The folowing text is no a number !! \n");
                        isNumber = false;
                    };

                    if (isNumber)
                    {
                        if(client_socket_list.find(socket_for_client, client_socket_information) < 0)
                        {
                            printf("=>Don't have this socket in the list \n");
                        }
                        else
                        {
                            std::unique_lock<std::mutex> locker_fd_set(fd_set_muxtex,std::defer_lock);

                            if (!container[0].compare("#"))
                            {
                                locker_fd_set.lock();
                                FD_CLR(socket_for_client, &master);
                                client_socket_list.remove(socket_for_client);    // delete client information.
                                close(socket_for_client);
                                input_fds.erase(std::remove(input_fds.begin(), input_fds.end(), socket_for_client), input_fds.end());
                                locker_fd_set.unlock();

                                printf("=> Closed connection with soket %d.\n",socket_for_client);
                            }
                            else
                            {
                                memset(&buffer,0,bufsize/sizeof(char));
                                strcpy(buffer, container[0].c_str());
                                total_bytes = container[0].length();

                                byte_left = total_bytes;
                                sended_bytes = 0;
                                int try_times = 0;

                                locker_fd_set.lock();
                                while(sended_bytes < total_bytes)
                                {
                                    send_fds = master;
                                    if (select(fdmax+1,nullptr, &send_fds, nullptr, &tv) <0)
                                    {
                                        perror("=>Select ");
                                        exit(EXIT_FAILURE);
                                    };

                                    if(FD_ISSET(socket_for_client, &send_fds))
                                    {
                                        status = send(socket_for_client, buffer+sended_bytes, byte_left, 0);
                                        if (status == -1UL)
                                        {
                                            printf("=>Sending failure !!!");
                                            if (try_times ++ < 3)
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
                                        printf("=> Socket is not ready to send data!! \n");
                                        if (try_times ++ < 3)
                                        {
                                            printf("=> Error on sending message");
                                            break;
                                        };
                                    };
                                };
                                locker_fd_set.unlock();
                            };
                        };
                    };
                }
                else
                {
                    printf("=> Wrong format message or no massge to send !! \n");
                };
            };
            user_command.clear();
        }
        else
        {
            sleep(1);
        };
    };
};

void process_on_buffer_recv(const char* buffer, client_list& client_socket_list, int input_fd, user_command& user_command){
    std::string bufstr = buffer;
    std::string message;
    std::vector<std::string> container;
    if (bufstr.compare("#")){
        splits_string(bufstr, container);
        if(container.size() == 2){
            if(!container[0].compare("*")){
                if(client_socket_list.set_user_name(input_fd,container[1].c_str()) < 0){
                    printf("=> Error on inserting username for new socket!! \n");
                };
            }else{
                int forward_fd = client_socket_list.get_by_user_name(container[1].c_str());
                if (forward_fd < 0){
                    message = "Sorry,"+container[1]+" hasn't connect to server!!"+"/"+std::to_string(input_fd);
                }else{
                    client_information host_message_client;
                    client_socket_list.find(input_fd, host_message_client);

                    std::string str(host_message_client.user_name);
                    message = str+"=>"+container[0]+"/"+std::to_string(forward_fd);
                }
                std::unique_lock<std::mutex> locker(user_command_muxtex);
                user_command.set(message);
                locker.unlock();
                cond.notify_all();
            };
        }else{
            printf("=> Wrong format stype recived massage \n");
        };
    };
};
