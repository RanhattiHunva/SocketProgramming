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
#include <stdlib.h>

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
    struct addrinfo *servinfor, *p;     // servinfor is linked-list which contain all address information.
                                        // p is used to query data.

    struct addrinfo hints;              // Clue to find the address information.
    memset(&hints, 0, sizeof(hints));   // To sure that hint is empty.
    hints.ai_family = AF_INET;          // Fill information for the hint. APF_INET: use IPv4,
    hints.ai_socktype = SOCK_STREAM;    // SOCK_STREAM: TCP/IP menthod.
    hints.ai_flags = AI_PASSIVE;        // AI_PASSIVE: assigned the address of local host tho the socket structure.

    int status;                         // Check error from getaddrinfor().Status return no-zero if there's an error.

    if ((status = getaddrinfo(nullptr, "1500", &hints, &servinfor)) !=0)
    {
        fprintf(stderr, "=> getaddinfo() error: %s", gai_strerror(status));
        fprintf(stderr, "%d", status);
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
    printf("   We're using the IPv4: %s - 1500\n", ipstr);\

    // Result will return ::: (IPv6) or 0:0:0:0 (IPv4) because this is the special
    // wildcard address which means to bind to all local interfaces.
    // which should i care about it? It still the question......


    /*------------------------------------------------------------------------------------------------------------*/
    /*  Creat socket depcristor
     *  Using socket to creat the socket decriptor. The socket hasn't assocate to any IP/PORT.
     *  int socket( int sockAF,      // PF_INET: IPv4, PF_INET6: IPv6.
     *              int socktype,    // SOCK_STREAM:TCP/IP, SOCK_DGRAM:UDP
     *              int protocol     // should be 0 to choose the proper protocol for the given socktype.)
     *  return -1 if there's an error.
     */

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

    /*  Manipulate some option of the socket to reuse address or port. */
//    int opt=1;
//    if ((setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) == -1 ){
//        perror("Setsockopt failed");
//        exit(EXIT_FAILURE);
//    }


    /*------------------------------------------------------------------------------------------------------------*/
    /*  Bind socket to an IP/PORT address.
     *  int bind( int sock_fd,                        // Socket decriptor.
     *            const struct sockaddr *addrinfo,    // Address that socket with bind with.
     *            socklen_t  addrLlen)                // Length of address.
     *                                                // socklen_t is unsigned integer.
     *  return -1 if there's an error.
     *  if using bind() with namespace std, there will have conflic.
     *  So we need to change into ::bind().
     */

    if ((::bind(server_fd, p->ai_addr, p->ai_addrlen))<0)
    {
        perror("=> Bind failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("=> Bind success!!\n");
    };
    printf("=> Server is ready to get datagram.... \n");


    /*------------------------------------------------------------------------------------------------------------*/
    /*  Start getting datagram from client.
     *
     */
    struct sockaddr_in client_addr;
    unsigned int size_client_address = sizeof(client_addr);

    const int bufsize = 1024;
    char buffer[bufsize];
    string inputStr;

    memset(&client_addr, 0, size_client_address);

    while(1){
        printf("Client: ");
        recvfrom(server_fd, buffer, bufsize, MSG_WAITALL,( struct sockaddr *) &client_addr, &size_client_address);
        cout << buffer << endl;

        printf("Server: ");
        getline(cin,inputStr);
        strcpy(buffer,inputStr.c_str());
        if (*buffer == '#'){
            break;
        } else {
            sendto(server_fd, buffer, bufsize, 0, ( struct sockaddr *) &client_addr, size_client_address);
        };
    };

    /*------------------------------------------------------------------------------------------------------------*/
    /*----- Close the server socket and exit the program -----*/
    freeaddrinfo(servinfor);
    close(server_fd);
    printf( "Closed the server socket!! \n\n");
}


