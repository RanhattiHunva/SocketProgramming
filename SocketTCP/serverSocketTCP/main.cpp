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
#include "iosocket.h"
#include "usercommand.h"

using namespace std;
std::mutex user_command_muxtex;
std::mutex fd_set_muxtex;
std::condition_variable cond;

class  scoped_thread        // Protecting thread. Sure that the threads are finsh for for the process is terminal.
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

int main()
{
    /* Notice the program information */
    printf("\n-------------**--------------\n\n");

    /*------------------------------------------------------------------------------------------------------------*/
    struct addrinfo *servinfor, *p;     // servinfor is linked-list which contain all address information.

    struct addrinfo hints;              // Clue to find the address information.
    memset(&hints, 0, sizeof(hints));   // To sure that hint is empty.
    hints.ai_family = AF_INET;          // Fill information for the hint. APF_INET: use IPv4,
    hints.ai_socktype = SOCK_STREAM;    // SOCK_STREAM: TCP/IP menthod.
    hints.ai_flags = AI_PASSIVE;        // AI_PASSIVE: assigned the address of local host tho the socket structure.

    long status;                         // Check error from getaddrinfor().Status return no-zero if there's an error.

    if ((status = getaddrinfo(nullptr, "1500", &hints, &servinfor)) !=0)
    {
        fprintf(stderr, "=> getaddinfo() error: %s", gai_strerror(static_cast<int>(status)));
        fprintf(stderr, "%ld",status);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("=> Gotten the computer's IP address infomation!!\n");
    };

    p = servinfor;  // Find the valid IP address by some information...But in this situation, the first one is good.

    struct sockaddr_in *addr_used = reinterpret_cast<struct sockaddr_in *>(p->ai_addr); // Show the IP infor to terminal
    void *addr_infor = &(addr_used -> sin_addr);
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(p->ai_family, addr_infor, ipstr, sizeof(ipstr));
    printf("   We're using the IPv4: %s - 1500\n", ipstr);

    /*------------------------------------------------------------------------------------------------------------*/
    // Creat file decriptor for the socket which use to open connection with client.
    int server_fd;
    if ((server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
    {
        perror("=> Socket failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("=> Created the server's socket decriptor!!\n");
    };

    /*------------------------------------------------------------------------------------------------------------*/
    // Connect the fd with a fix address.
    if ((::bind(server_fd, p->ai_addr, p->ai_addrlen))<0)
    {
        perror("=> Bind failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("=> Bind success!!\n");
    };
    freeaddrinfo(servinfor);

    /*------------------------------------------------------------------------------------------------------------*/
    // Server_fd start to listen the upcomming client.
    if (listen(server_fd, 10) < 0)
    {
        perror("=> Listen failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("=> Server's listening!! \n");
    };

    /*------------------------------------------------------------------------------------------------------------*/
    // Connect and communiactio with client.
    int socket_for_client;              // Variable to save client socket data.
    struct sockaddr client_addr;
    unsigned int size_client_address = sizeof(client_addr);
    char IPclient[INET6_ADDRSTRLEN];
    client_list client_socket_list;     // Search client_list's properties in client.h
    std::vector <int> input_fds;

    const unsigned int bufsize = 1024;
    char buffer[bufsize];

    fd_set master;
    fd_set read_fds;

    const int stdin = 0;    // stdin = 0 ifs the file decriptors of in/output terminal.

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    input_fds.clear();

    FD_SET(server_fd, &master);
    FD_SET(stdin,&master);
    input_fds.push_back(stdin);
    input_fds.push_back(server_fd);

    struct timeval tv;          // waiting time for select()
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    int fdmax = (server_fd > stdin) ? server_fd : stdin;

    // Set non-blocking
    long arg = fcntl(server_fd, F_GETFL, NULL);
    arg |= O_NONBLOCK;
    fcntl(server_fd, F_SETFL, arg);

    user_command user_command;   // object to project race-conditon on user input from terminal.
    user_command.clear();        // search properties on usercomnand.h

    scoped_thread sendThread(std::thread(send_TCP,std::ref(user_command), std::ref(client_socket_list), std::ref(master), std::ref(fdmax), std::ref(input_fds)));

    while(!user_command.compare("#"))
    {
        std::unique_lock<mutex> locker_fd_set(fd_set_muxtex,std::defer_lock);
        locker_fd_set.lock();
        read_fds = master;

        if (select(fdmax+1,&read_fds, nullptr, nullptr, &tv) <0)
        {
            perror("=> Select");
            exit(EXIT_FAILURE);
        }

        locker_fd_set.unlock();

        for (unsigned long i = 0; i< input_fds.size(); i++)
        {
            if(FD_ISSET(input_fds[i], &read_fds))
            {
                if( input_fds[i] == stdin)    // Get user input from terminal.
                {
                    string inputStr;
                    getline(cin, inputStr);

                    std::unique_lock<mutex> locker(user_command_muxtex);
                    user_command.set(inputStr);
                    locker.unlock();
                    cond.notify_all();
                }
                else if( input_fds[i] == server_fd)   // Get new connection from new client.
                {
                    if((socket_for_client = accept(server_fd, &client_addr, &size_client_address))<0)
                    {
                        perror("=> Accept error");
                    }
                    else
                    {
                        FD_SET(socket_for_client, &master);
                        input_fds.push_back(socket_for_client);

                        if (socket_for_client > fdmax)
                        {
                            fdmax = socket_for_client;
                        };

                        inet_ntop(client_addr.sa_family,&(reinterpret_cast<struct sockaddr_in* >(&client_addr)->sin_addr),IPclient, INET6_ADDRSTRLEN);
                        client_socket_list.add(IPclient,socket_for_client);  // Saving client information to client list.

                        printf("=> New conection from %s on socket %d\n",IPclient,socket_for_client);

                        fcntl(socket_for_client, F_SETFL, O_NONBLOCK); // Put new socket into non-locking state
                    };
                }
                else    // Get data from an client.
                {
                    memset(&buffer,0,bufsize);
                    if ((status=recv(input_fds[i], buffer, bufsize,0)) <=0)
                    {
                        if ((status == -1) && ((errno == EAGAIN)|| (errno == EWOULDBLOCK)))
                        {
                            perror("=> Message is not gotten complete !!");
                        }
                        else
                        {
                            if (status == 0)
                            {
                                printf("=> Connect client has been close, soket: %d\n",input_fds[i]);
                            }
                            else
                            {
                                printf("=> Error socket.");
                            };

                            locker_fd_set.lock();

                            FD_CLR(input_fds[i], &master);
                            client_socket_list.remove(input_fds[i]);    // delete client information.
                            close(input_fds[i]);
                            input_fds.erase(input_fds.begin()+static_cast<long>(i));

                            locker_fd_set.unlock();
                        };
                    }
                    else
                    {
                        cout << "Messaga from client, socket " << input_fds[i] << ":" << buffer << endl;
                        process_on_buffer_recv(buffer,client_socket_list, input_fds[i], user_command);
                    };
                };
            };
        };
    };
    /*------------------------------------------------------------------------------------------------------------*/
    close(server_fd);
    printf("Closed the server socket!! \n");
}


