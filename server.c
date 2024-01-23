#include <netinet/in.h>
#include <time.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>   

#define MAXLINE     4096    /* max text line length */
#define LISTENQ     1024    /* 2nd argument to listen() */
#define DAYTIME_PORT 3333

int
main(int argc, char **argv)
{
    int     listenfd, connfd;
    struct sockaddr_in servaddr;
    char    buff[MAXLINE];
    time_t ticks;


    if (argc != 2) {
        printf("usage: server <Portnumber>\n");
        exit(1);
    }

    long port_num= atoi(argv[1]);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    //set the entire structure to 0 using bzero to avoid uninitialized data
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    //IP address as INADDR_ANY, which allows the server to accept a client connection 
    //on any interface, in case the server host has multiple interfaces
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((uint16_t)port_num); /* daytime server */

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind error");
        exit(1);
    }

    //calling listen(), the socket is converted into a listening socket, 
    //on which incoming connections from clients will be accepted by the kernel.
    if (listen(listenfd, LISTENQ) < 0) {
        perror("listen error");
        exit(1);
    }
    
    printf("Server listening on port %ld...\n", port_num);
    for ( ; ; ) {
        connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
        if (connfd < 0) {
            perror("accept error");
            continue;
        }

        printf("Accepted connection. Sending response...\n");

        ticks = time(NULL);
        //appended to the string by snprintf
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks)); //ctime() ticks value into a human-readable
        //the result is written to the client by write
        write(connfd, buff, strlen(buff));
        printf("Sending response: %s", buff);

        //Terminate connection
        close(connfd);
        printf("Connection closed.\n");
    }
}

