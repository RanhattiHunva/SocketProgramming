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

#include "usercommand.h"

using namespace std;
static std::mutex user_command_muxtex;
static std::condition_variable cond;

class scoped_thread
{
    std::thread t;
public:
    explicit scoped_thread(std::thread t_):t(std::move(t_))
    {
        if (!t.joinable())
        {
            throw std::logic_error("No thread");
        }
    }
    ~scoped_thread()
    {
        t.join();
    }
    scoped_thread(scoped_thread const&)=delete;
    scoped_thread& operator = (scoped_thread const&) = delete;
};


void send_TCP(user_command& user_command, fd_set& master, int& fdmax, int& socket_fd)
{
    string inputStr;
    inputStr.clear();

    const unsigned int bufsize = 1024;
    char buffer[bufsize];

    unsigned long total_bytes, byte_left, status, sended_bytes;

    fd_set send_fds;
    FD_ZERO(&send_fds);

    struct timeval general_tv;
    general_tv.tv_sec = 1;
    general_tv.tv_usec = 0;

    std::string user_cmd_str;

    while(1)
    {
        std::unique_lock<std::mutex> locker(user_command_muxtex);
        cond.wait(locker);

        if (!user_command.empty())
        {
            user_cmd_str = user_command.get();
            locker.unlock();

            if(!user_cmd_str.compare("#"))
            {
                break;
            }
            else
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
                    if (select(fdmax+1,nullptr, &send_fds, nullptr, &general_tv) <0)
                    {
                        perror("=>Select ");
                        exit(EXIT_FAILURE);
                    };

                    if(FD_ISSET(socket_fd, &send_fds))
                    {
                        status = send(socket_fd, buffer+sended_bytes, byte_left, 0);
                        if (status == -1UL)
                        {
                            printf("=> Sending failure !!!");
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
            };
        }
        else
        {
            sleep(1);
        };
    };
};

int main()
{
    /* Notice the program information */
    printf("\n-------------**--------------\n\n");
    printf("=> Please inser user's names before start program:");
    string user_name;
    getline(cin, user_name);

    /*------------------------------------------------------------------------------------------------------------*/
    struct addrinfo *clientinfor, *p;   // servinfor is linked-list which contain all address information.
    // p is used to query data.

    struct addrinfo hints;              // Clue to find the address information.
    memset(&hints, 0, sizeof(hints));   // To sure that hint is empty.
    hints.ai_family = AF_INET;          // Fill information for the hint. APF_INET: use IPv4,
    hints.ai_socktype = SOCK_STREAM;    // SOCK_STREAM: TCP/IP menthod.
    hints.ai_flags = AI_PASSIVE;        // AI_PASSIVE: assigned the address of local host tho the socket structure.

    long status;                         // Check error from getaddrinfor().Status return no-zero if there's an error.

    if ((status = getaddrinfo(nullptr, "1500", &hints, &clientinfor)) !=0)
    {
        fprintf(stderr, "=> getaddinfo() error: %s", gai_strerror(static_cast<int>(status)));
        fprintf(stderr, "%d", static_cast<int>(status));
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("=> Gotten the computer's IP address infomation!!\n");
    };

    p = clientinfor;  // Find the valid IP address by some information...But in this situation, the first one is good.
    /*------------------------------------------------------------------------------------------------------------*/
    int client_fd;                    // file decriptor to save linked-list.
    if ((client_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
    {
        perror("=> Socket failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("=> Created the server's socket decriptor!!\n");
    };
    freeaddrinfo(clientinfor);

    /*------------------------------------------------------------------------------------------------------------*/
    const int portNum = 1500;               // Creat the server infor.
    struct sockaddr_in server_addr;         // In this situation the server's IP is also the IP of computer
    server_addr.sin_family = AF_INET;       // so server_addr is very simple. But with IP of a website we need to get it through
    server_addr.sin_port = htons(portNum);  // getaddrinfo().

    // Set non-blocking
    long arg = fcntl(client_fd, F_GETFL, NULL);
    arg |= O_NONBLOCK;
    fcntl(client_fd, F_SETFL, arg);

    fd_set master;
    fd_set send_fds;
    fd_set read_fds;

    FD_ZERO(&master);
    FD_SET(client_fd, &master);

    // Trying to connect with timeout
    status = connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (status < 0)
    {
        if (errno == EINPROGRESS)
        {
            struct timeval connect_delays;
            connect_delays.tv_sec = 3;
            connect_delays.tv_usec = 0;
            send_fds = master;

            if(select(client_fd+1, nullptr, &send_fds, nullptr, &connect_delays) > 0)
            {
                int error_check;
                socklen_t lon = sizeof(error_check);
                getsockopt(client_fd, SOL_SOCKET, SO_ERROR, (void*)(&error_check), &lon);
                if (error_check)
                {
                    fprintf(stderr, "=> Error in connection() %d - %s\n", error_check, strerror(error_check));
                    exit(EXIT_FAILURE);
                };
            }
            else
            {
                printf("=> Connection timeout");
                exit(EXIT_FAILURE);
            };
        }
        else
        {
            printf("=> Error in connection()");
            exit(EXIT_FAILURE);
        };
    };
    printf("=> Connection to the server port number: %d",portNum);

    /*------------------------------------------------------------------------------------------------------------*/
    printf("\n=> Enter # to end the connection \n\n");

    user_command user_command;   // object to project race-conditon on user input from terminal.
    user_command.clear();        // search properties on usercomnand.h

    const int stdin = 0;
    FD_SET(stdin,&master);
    int fdmax = (client_fd > stdin) ? client_fd : stdin;

    struct timeval general_tv;
    general_tv.tv_sec = 1;
    general_tv.tv_usec = 0;

    scoped_thread sendThread(std::thread(send_TCP,std::ref(user_command), std::ref(master), std::ref(fdmax), std::ref(client_fd)));

    sleep(1);
    std::unique_lock<mutex> locker(user_command_muxtex);
    user_command.set("*/"+user_name);
    locker.unlock();
    cond.notify_all();

    const unsigned int bufsize = 1024;
    char buffer[bufsize];

    while(!user_command.compare("#"))
    {
        read_fds = master;
        if (select(fdmax+1,&read_fds, nullptr, nullptr, &general_tv) <0)
        {
            perror("=> Select");
            exit(EXIT_FAILURE);
        }

        if(FD_ISSET(stdin, &read_fds))
        {
            string inputStr;
            getline(cin, inputStr);

            std::unique_lock<mutex> locker(user_command_muxtex);
            user_command.set(inputStr);
            locker.unlock();
            cond.notify_all();

        }
        else if (FD_ISSET(client_fd, &read_fds))
        {
            memset(&buffer,0,bufsize);
            if ((status=recv(client_fd,buffer, bufsize,0)) <=0 )
            {
                if ((status == -1) && ((errno == EAGAIN)|| (errno == EWOULDBLOCK)))
                {
                    perror("=> Message is not gotten complete !!");
                }
                else
                {
                    if (status == 0)
                    {
                        printf("=> Connection has been close\n");
                    }
                    else
                    {
                        printf("=> Error socket.");
                    };

                    std::unique_lock<mutex> locker(user_command_muxtex);
                    user_command.set("#");
                    locker.unlock();
                    cond.notify_all();
                    break;
                };
            }
            else
            {
                cout << "Messaga from server :" << buffer << endl;
            };
        };
    };
    /*------------------------------------------------------------------------------------------------------------*/
    /*----- Close the server socket and exit the program -----*/
    printf("\n=> Socket is closed.\n=> Goodbye...\n");
    close(client_fd);
    return 0;
}
