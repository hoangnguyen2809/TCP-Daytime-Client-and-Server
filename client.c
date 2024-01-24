#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE     4096    /* max text line length */
#define DAYTIME_PORT 3333

struct message{
    int addrlen, timelen, msglen;
    char addr[MAXLINE];
    char currtime[MAXLINE];
    char payload[MAXLINE];
};

//Function extracted from "Unix Network Programming: The Sockets Networking API" by Stevens, Fenner, Rudoff 
int readable_timeo(int fd, int sec)
{
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    tv.tv_sec = sec;
    tv.tv_usec = 0;

    return (select(fd+1, &rset, NULL, NULL, &tv));    
}

//Functions abstracted from "Unix Network Programming: The Sockets Networking API" by Stevens, Fenner, Rudoff (2003)
int tcp_connect(const char *host, const char *port)
{
    int sockfd, n;
    struct addrinfo hints, *res, *ressave;
    char host_buffer[NI_MAXHOST], service_buffer[NI_MAXSERV];

    bzero(&hints, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((n = getaddrinfo (host, port, &hints, &res)) != 0)
    {
        fprintf(stderr,"tcp_connect error for %s, %s: %s",host, port, gai_strerror (n));
        exit(1);
    }
    ressave = res;

    do {
        sockfd = socket (res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue;
        if (connect (sockfd, res->ai_addr, res->ai_addrlen) == 0)
            break;
        close(sockfd);
    } while ( (res = res->ai_next) != NULL);

    if (res == NULL)
    {
        fprintf (stderr,"tcp_connect error for %s, %s", host, port);
        exit(1);
    }

    if (getnameinfo(res->ai_addr, res->ai_addrlen, host_buffer, sizeof(host_buffer),
                service_buffer, sizeof(service_buffer), 0) == 0)
    {
        printf("Server name: %s\n", host_buffer);
    }
    else
    {
        perror("getnameinfo error");
    }


    freeaddrinfo (ressave);

    return (sockfd);
}

int Tcp_connect (const char *host, const char *port)
{
    return (tcp_connect (host, port));
}

int main(int argc, char **argv)
{
    int     sockfd, n;
    struct message received_msg;

    if (argc != 3) {
        fprintf(stderr, "Usage: client <ServerHostname/IPaddress> <PortNumber>\n");
        exit(1);
    }

    sockfd = Tcp_connect (argv[1], argv[2]);

    //     //The socket() function creates an Internet (AF_INET) stream (SOCK_STREAM) socket => TCP
    // if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    //     //If the call to socket fails, we abort the program
    //     printf("socket error\n");
    //     exit(1);
    // }

    // //set the entire structure to 0 using bzero to avoid uninitialized data
    // bzero(&servaddr, sizeof(servaddr));
    // servaddr.sin_family = AF_INET;
    // servaddr.sin_port = htons((uint16_t)port_num); //hton() to convert the binary portnumber
    // //set the IP address to the value of (argv[1])
    // //inet_pton to convert the ASCII command-line argument into the proper format.
    // if (inet_pton(AF_INET, ip_address, &servaddr.sin_addr) <= 0) {
    //     printf("inet_pton error for %s\n", argv[1]);
    //     exit(1);
    // }

    // //establishes a TCP connection with the server specified by the socket address structure 
    // //pointed to by the second argument
    // if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
    //     printf("connect error\n");
    //     exit(1);
    // }
    

    //read the server’s reply and display the result 
    while ((n = read(sockfd, &received_msg, sizeof(struct message))) > 0) {
        printf("IP Address: %.*s\n", received_msg.addrlen, received_msg.addr);
        printf("Time: %.*s", received_msg.timelen, received_msg.currtime);
        printf("Who: %.*s\n", received_msg.msglen, received_msg.payload);
    }
    if (n < 0) {
        printf("read error\n");
        exit(1);
    }

    exit(0);
}

