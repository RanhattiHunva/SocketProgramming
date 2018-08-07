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

using namespace std;
int main()
{
    /* Notice the program information */
    printf("\n-------------**--------------\n\n");


    /*------------------------------------------------------------------------------------------------------------*/
    /*  Get the overall the computer's address information and save it as a linked-list of
     *  struct addinformation.
     *
     *  int getaddrinfo( const char *node,               // IP or domain of website.
     *                   const char *service,            // Port or name of the needed service: "http",ftp",...
     *                   const struct addinfo *hint,     // Clue to define the needed IP address.
     *                   struct addrinfo **res)          // Pointer to save linked-list.
     */
    struct addrinfo *clientinfor, *p;   // servinfor is linked-list which contain all address information.
                                        // p is used to query data.

    struct addrinfo hints;              // Clue to find the address information.
    memset(&hints, 0, sizeof(hints));   // To sure that hint is empty.
    hints.ai_family = AF_INET;          // Fill information for the hint. APF_INET: use IPv4,
    hints.ai_socktype = SOCK_STREAM;    // SOCK_STREAM: TCP/IP menthod.
    hints.ai_flags = AI_PASSIVE;        // AI_PASSIVE: assigned the address of local host tho the socket structure.

    int status;                         // Check error from getaddrinfor().Status return no-zero if there's an error.

    if ((status = getaddrinfo(nullptr, "1500", &hints, &clientinfor)) !=0)
    {
        fprintf(stderr, "=> getaddinfo() error: %s", gai_strerror(status));
        fprintf(stderr, "%d", status);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("=> Gotten the computer's IP address infomation!!\n");
    };

    p = clientinfor;  // Find the valid IP address by some information...But in this situation, the first one is good.


    /*------------------------------------------------------------------------------------------------------------*/
    /*  Creat socket depcristor
     *  Using socket to creat the socket decriptor. The socket hasn't assocate to any IP/PORT.
     *  int socket( int sockAF,      // PF_INET: IPv4, PF_INET6: IPv6.
     *              int socktype,    // SOCK_STREAM:TCP/IP, SOCK_DGRAM:UDP
     *              int protocol     // should be 0 to choose the proper protocol for the given socktype.)
     *  return -1 if there's an error.
     */

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


    /*------------------------------------------------------------------------------------------------------------*/
    /*  Connect to server.
     *  int connect( int client_fd,                             // Socket decriptor of client
     *               const struct sockaddr * server_addr,       // Server address information
     *               socklen_t  addrlen)                        // Length of server's adrress.
     */
    const int portNum = 1500;               // Creat the server infor.
    struct sockaddr_in server_addr;         // In this situation the server's IP is also the IP of computer
    server_addr.sin_family = AF_INET;       // so server_addr is very simple. But with IP of a website we need to get it through
    server_addr.sin_port = htons(portNum);  // getaddrinfo().

    if (connect(client_fd,(struct sockaddr *)&server_addr, sizeof(server_addr)) == 0) // old-style cast????
        cout << "=> Connection to the server port number: " << portNum << endl;


    /*------------------------------------------------------------------------------------------------------------*/
    /*  Stating communication with client.
     *
     *  int send( int sockfd,                   // Socket decriptor which server create
     *                                          // to connect with client.
     *            const void *buff,             // Array buffer
     *            int bufflen,                  // Length array buffer.
     *            int flag)                     // Should be 0 to get normal data.
     *
     *  int recv( int sockfd,                   // Socket decriptor which server create
     *                                          // to connect with client.
     *            const void *buff,             // Array buffer
     *            int bufflen,                  // Length array buffer.
     *            int flag)                     // Should be 0 to get normal data.
     */

    const int bufsize = 1024;
    char buffer[bufsize];
    bool isExit = false;

    cout << "=> Awaiting confirmation from the server..." << endl;  // Too sure that connection is establish
    recv(client_fd, buffer, bufsize, 0);                            // client will wait until get a first message from server.
    cout << "=> Connection confirmed, you are good to go...";
    cout << "\n=> Enter # to end the connection\n" << endl;

    do {                                        // Starting communication ....
        cout << "Client: ";
        do {
            cin >> buffer;
            send(client_fd, buffer, bufsize, 0);
            if (*buffer == '#') {
                send(client_fd, buffer, bufsize, 0);
                *buffer = '*';
                isExit = true;
            }
        } while (*buffer != 42);

        cout << "Server: ";
        do {
            recv(client_fd, buffer, bufsize, 0);
            cout << buffer << " ";
            if (*buffer == '#') {
                *buffer = '*';
                isExit = true;
            }

        } while (*buffer != 42);
        cout << endl;

    } while (!isExit);


    /*------------------------------------------------------------------------------------------------------------*/
    /*----- Close the server socket and exit the program -----*/
    cout << "\n=> Connection terminated.\nGoodbye...\n";
    freeaddrinfo(clientinfor);
    close(client_fd);
    return 0;
}
