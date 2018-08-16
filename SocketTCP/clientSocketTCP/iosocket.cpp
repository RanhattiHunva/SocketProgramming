#include "iosocket.h"

void send_TCP(user_command& user_command, fd_set& master, int& socket_fd)
{
    const unsigned int bufsize = 1024;
    char buffer[bufsize];

    unsigned long total_bytes, byte_left, sended_bytes;
    long status;

    fd_set send_fds;
    FD_ZERO(&send_fds);

    struct timeval general_tv;
    general_tv.tv_sec = 1;
    general_tv.tv_usec = 0;

    std::string user_cmd_str;
    bool is_first_run = true;

    while(1)
    {
        if (is_first_run)
        {
            user_cmd_str = "*/"+user_name;
            is_first_run = false;
        }
        else
        {
            if(user_command.compare("#"))
            {
                break;
            }
            else
            {
                getline(std::cin, user_cmd_str);
                user_command.set(user_cmd_str);
            };
        };

        if (!user_cmd_str.empty())
        {
            if(user_cmd_str.compare("#"))
            {
                memset(&buffer,0,bufsize/sizeof(char));
                strcpy(buffer, user_cmd_str.c_str());
                total_bytes = user_cmd_str.length();

                byte_left = total_bytes;
                sended_bytes = 0;
                int try_times = 0;

                while(sended_bytes < total_bytes)
                {
                    send_fds = master;
                    if (select(socket_fd+1,nullptr, &send_fds, nullptr, &general_tv) <0)
                    {
                        perror("=>Select ");
                        exit(EXIT_FAILURE);
                    };

                    if(FD_ISSET(socket_fd, &send_fds))
                    {
                        status = send(socket_fd, buffer+sended_bytes, byte_left, 0);
                        if (status < 0)
                        {
                            printf("=> Sending failure !!!");
                            if (try_times ++ < 3)
                            {
                                break;
                            };
                        }
                        else
                        {
                            sended_bytes += static_cast<unsigned long>(status);
                            byte_left -= static_cast<unsigned long>(status);
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
            }
            else
            {
                break;
            };
        }
        else
        {
            sleep(1);
        };
    };
};

int is_reconnect(int& client_fd)
{
    std::string answer;
    while (1)
    {
        printf("=> Do you want reconnect to server (Y/N)?\n");
        getline(std::cin, answer);

        if(!answer.compare("Y"))
        {
            close(client_fd);
            return 0;
        }
        else if (!answer.compare("N"))
        {
            close(client_fd);
            return -1;
        }
        else
        {
            printf("=> Don't understand the answer.\n");
            continue;
        }
    };
};

int connect_with_timeout(struct addrinfo *p, fd_set& master, int& client_fd)
{
    if ((client_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
    {
        perror("=> Socket failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("=> Created a client's socket decriptor!!\n");
    };

    long arg = fcntl(client_fd, F_GETFL, NULL);
    arg |= O_NONBLOCK;
    fcntl(client_fd, F_SETFL, arg);

    fd_set send_fds;

    struct timeval connect_delays;
    connect_delays.tv_sec = 3;
    connect_delays.tv_usec = 0;

    FD_ZERO(&master);
    FD_SET(client_fd, &master);

    long status = connect(client_fd, p->ai_addr,  p->ai_addrlen);
    if (status < 0)
    {
        if (errno == EINPROGRESS)
        {
            send_fds = master;

            if(select(client_fd+1, nullptr, &send_fds, nullptr, &connect_delays) > 0)
            {
                int error_check;
                socklen_t lon = sizeof(error_check);
                getsockopt(client_fd, SOL_SOCKET, SO_ERROR, (void*)(&error_check), &lon);
                if (error_check)
                {
                    fprintf(stderr, "=> Error in connection() %d - %s\n", error_check, strerror(error_check));
                    return -1;
                }
                else
                {
                    return 0;
                }
            }
            else
            {
                printf("=> Connection timeout \n");
                return -1;
            };
        }
        else
        {
            printf("=> Error in connection()\n");
            return -1;
        };
    }
    else
    {
        return 0;
    };
};








