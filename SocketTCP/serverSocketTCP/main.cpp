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

std::mutex user_command_muxtex;
std::condition_variable cond;

using namespace std;

class  scoped_thread        // Protecting thread. Sure that the threads are finsh for for the process is terminal.
{
    std::thread t;
public:
    explicit scoped_thread(std::thread t_):t(std::move(t_)) {}
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
    int server_fd;                    // file decriptor to save linked-list.
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
    int socket_for_client;      // Variable to save client socket data.
    struct sockaddr client_addr;
    unsigned int size_client_address = sizeof(client_addr);
    char IPclient[INET6_ADDRSTRLEN];
    client_list client_socket_list;     // search client_list's properties in client.h
    std::vector <int> client_file_dercriptors;

    const unsigned int bufsize = 1024;
    char buffer[bufsize];

    fd_set master;
    fd_set read_fds;
    client_file_dercriptors.clear();

    const int stdin = 0;        // stdin = 0 ifs the file decriptors of in/output terminal.

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    client_file_dercriptors.push_back(stdin);
    client_file_dercriptors.push_back(server_fd);

    FD_SET(server_fd, &master);
    FD_SET(stdin,&master);

    struct timeval tv;          // waiting time for select()
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    int fdmax;
    if (server_fd > stdin)
    {
        fdmax = server_fd;
    }
    else
    {
        fdmax = stdin;
    }

    fcntl(server_fd, F_SETFL, O_NONBLOCK);     // set socket to be un-locking state.

    user_command userCommand;   // object to project race-conditon on user input from terminal.
    userCommand.clear();        // search properties on usercomnand.h

    scoped_thread sendThread(std::thread(sendTCP,std::ref(userCommand), std::ref(client_socket_list), std::ref(master), std::ref(fdmax)));
    // for more about sendTCP() please search on iosoket.h

    while(!userCommand.compare("#"))
    {
        read_fds = master;
        if (select(fdmax+1,&read_fds, nullptr, nullptr, &tv) <0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }
        unsigned long number_file_decriptors = client_file_dercriptors.size();

        for (unsigned long i = 0; i< number_file_decriptors; i++)
        {
            if(FD_ISSET(client_file_dercriptors[i], &read_fds))
            {
                if( client_file_dercriptors[i] == stdin)               // get user input from terminal.
                {
                    string inputStr;
                    getline(cin, inputStr);

                    std::unique_lock<mutex> locker(user_command_muxtex);
                    userCommand.set(inputStr);
                    locker.unlock();
                    cond.notify_one();

                }
                else if( client_file_dercriptors[i] == server_fd)     // get new connection from new client.
                {
                    if((socket_for_client = accept(server_fd, &client_addr, &size_client_address))<0)
                    {
                        perror("=> Accept error");
                    }
                    else
                    {
                        FD_SET(socket_for_client, &master);
                        client_file_dercriptors.push_back(socket_for_client);

                        if (socket_for_client > fdmax)
                        {
                            fdmax = socket_for_client;
                        };

                        inet_ntop(client_addr.sa_family,&(reinterpret_cast<struct sockaddr_in* >(&client_addr)->sin_addr),IPclient, INET6_ADDRSTRLEN);
                        client_socket_list.addElement(IPclient,socket_for_client);  // saving client information to client list.

                        printf("=> New conection from %s on socket %d\n",IPclient,socket_for_client);

                        fcntl(socket_for_client, F_SETFL, O_NONBLOCK); // put new socket into non-locking state
                    };
                }
                else                        // get data from an client.
                {
                    memset(&buffer,0,bufsize);
                    if ((status=recv(client_file_dercriptors[i],buffer, bufsize,0)) <=0 )
                    {
                        if (status == 0)
                        {
                            printf("=>Connect client has been close, soket: %d\n",i);
                        }
                        else
                        {
                            perror("=> Recv error!!\n");
                        };
                        FD_CLR(client_file_dercriptors[i], &master);
                        client_socket_list.removeElement(client_file_dercriptors[i]);    // delete client information.
                        client_file_dercriptors.erase(client_file_dercriptors.begin()+static_cast<long>(i));

                        close(client_file_dercriptors[i]);
                    }
                    else
                    {
                        cout << "Messaga from client, socket " << client_file_dercriptors[i] << ":" << buffer << endl;
                    };
                };
            };
        };
    };
    /*------------------------------------------------------------------------------------------------------------*/
    close(server_fd);
    printf("Closed the server socket!! \n");
}


