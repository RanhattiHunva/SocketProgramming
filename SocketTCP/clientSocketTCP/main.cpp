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
#include "iosocket.h"

using namespace std;
string user_name;

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

int main()
{
    /* Notice the program information */
    printf("\n-------------**--------------\n\n");
    printf("=> Please inser user's names before start program:");
    getline(cin, user_name);

    /*------------------------------------------------------------------------------------------------------------*/
    struct addrinfo *servinfor, *p;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    long status;

    if ((status = getaddrinfo(nullptr, "1500", &hints, &servinfor)) !=0)
    {
        fprintf(stderr, "=> getaddinfo() error: %s", gai_strerror(static_cast<int>(status)));
        fprintf(stderr, "%d", static_cast<int>(status));
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("=> Gotten the computer's IP address infomation!!\n");
    };

    p = servinfor;
    /*------------------------------------------------------------------------------------------------------------*/

    bool is_finsh = false;
    while (!is_finsh)
    {
        int client_fd;
        fd_set read_fds;
        fd_set master;

        while((status=connect_with_timeout(p,master,client_fd))<0)
        {
            int flage_reconnect = is_reconnect(client_fd);
            if (flage_reconnect < 0)
            {
                exit(EXIT_FAILURE);
            }
        }

        /*------------------------------------------------------------------------------------------------------------*/
        printf("\n=> Enter # to end the connection \n\n");

        user_command user_command;   // object to project race-conditon on user input from terminal.
        user_command.clear();        // search properties on usercomnand.h

        struct timeval general_tv;
        general_tv.tv_sec = 1;
        general_tv.tv_usec = 0;

        scoped_thread sendThread(std::thread(send_TCP,std::ref(user_command), std::ref(master), std::ref(client_fd)));
        const unsigned int bufsize = 1024;
        char buffer[bufsize];

        while(!user_command.compare("#"))
        {
            read_fds = master;
            if (select(client_fd+1,&read_fds, nullptr, nullptr, &general_tv) <0)
            {
                perror("=> Select");
                exit(EXIT_FAILURE);
            }

            if (FD_ISSET(client_fd, &read_fds))
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
                        user_command.set("#");
                        break;
                    };
                }
                else
                {
                    cout << "Messaga from server :" << buffer << endl;
                };
            };
        };
        is_finsh = is_reconnect(client_fd);
    };

    /*------------------------------------------------------------------------------------------------------------*/
    /*----- Close the server socket and exit the program -----*/
    freeaddrinfo(servinfor);
    printf("\n=> Socket is closed.\n=> Goodbye...\n");
//    close(client_fd);
    return 0;
}
