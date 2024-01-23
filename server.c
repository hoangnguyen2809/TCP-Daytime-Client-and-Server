#include <netinet/in.h>
#include <arpa/inet.h>
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

//Populate fields of message before sending
void fill_message(struct message *msg, const char *ip_address, const char *current_time, const char *payload) {
    msg->addrlen = strlen(ip_address);
    msg->timelen = strlen(current_time);
    msg->msglen = strlen(payload);

    strncpy(msg->addr, ip_address, sizeof(msg->addr));
    strncpy(msg->currtime, current_time, sizeof(msg->currtime));
    strncpy(msg->payload, payload, sizeof(msg->payload));

    msg->addr[msg->addrlen] = '\0';
    msg->currtime[msg->timelen] = '\0';
    msg->payload[msg->msglen] = '\0';
}

int main(int argc, char **argv)
{
    int     listenfd, connfd;
    struct sockaddr_in servaddr;
    socklen_t addr_len = sizeof(servaddr);
    struct message my_message;
    char    buff[MAXLINE];
    time_t ticks;


    if (argc != 2) {
        printf("usage: server <Portnumber>\n");
        exit(1);
    }

    long port_num = atoi(argv[1]);

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

        ticks = time(NULL);
        //appended current time to the string buff
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks)); //ctime() ticks value into a human-readable
        

        //initilize message
        char server_ip[INET_ADDRSTRLEN]; //retrieve server ip address
        //obtain the local address through getsockname
        if (getsockname(connfd, (struct sockaddr *)&servaddr, &addr_len) == 0) 
        {
            inet_ntop(AF_INET, &(servaddr.sin_addr), server_ip, INET_ADDRSTRLEN);
        }
        else {
            perror("getsockname error");
        }
        
        const char *payload = "";
        fill_message(&my_message, server_ip, buff, payload);

        
        //the message is copied to connfd and send back to client
        printf("Sending response: %s", buff);
        write(connfd, &my_message, sizeof(struct message));
        

        //Terminate connection
        close(connfd);
        printf("Connection closed.\n");
    }
}

