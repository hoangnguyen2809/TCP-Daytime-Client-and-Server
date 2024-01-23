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

struct message{
    int addrlen, timelen, msglen;
    char addr[MAXLINE];
    char currtime[MAXLINE];
    char payload[MAXLINE];
};

int main(int argc, char **argv)
{
    int     listenfd, connfd, n;
    struct sockaddr_in servaddr;
    struct message received_msg;
    char    buff[MAXLINE];
    time_t ticks;


    if (argc != 2) {
        printf("usage: server <Portnumber>\n");
        exit(1);
    }

    long port_num= atoi(argv[1]);

    //creating a TCP socket in server side
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    //set the entire structure to 0 using bzero to avoid uninitialized data
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    //IP address as INADDR_ANY, which allows the server to accept a client connection 
    //on any interface, in case the server host has multiple interfaces
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((uint16_t)port_num); /* daytime server */

    //bind the socket listenfd to the server address
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
    for ( ; ; ) 
    {
        //if there is a request, accept the request
        connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
        if (connfd < 0) 
        {
            perror("accept error");
            continue;
        }

        printf("Accepted connection.\n");

        n = read(connfd, &received_msg, sizeof(struct message));
        if (n < 0)
        {
            printf("read error\n");
            close(connfd);
            exit(1);
        }

        printf("Received Message:\n");
        printf("Address: %.*s\n", received_msg.addrlen, received_msg.addr);
        printf("Time: %.*s\n", received_msg.timelen, received_msg.currtime);
        //printf("Payload: %.*s\n", received_msg.msglen, received_msg.payload);

        ticks = time(NULL);
        //appended current time to the string buff
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks)); //ctime() ticks value into a human-readable
        //the result is copied to connfd and send back to client
        printf("Sending response: %s", buff);
        write(connfd, buff, strlen(buff));
        

        //Terminate connection
        close(connfd);
        printf("Connection closed.\n");
    }
}

