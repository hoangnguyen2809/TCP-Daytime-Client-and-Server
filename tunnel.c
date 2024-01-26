#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>   

#define MAXLINE     4096    /* max text line length */
#define LISTENQ     1024    /* 2nd argument to listen() */

struct message{
    int addrlen, timelen, msglen;
    char addr[MAXLINE];
    char currtime[MAXLINE];
    char payload[MAXLINE];
};


//Functions abstracted from "Unix Network Programming: The Sockets Networking API" by Stevens, Fenner, Rudoff (2003)
int tcp_connect(const char *host, const char *port)
{
    int sockfd, n;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;


    //getaddrinfo handles both name-to-address and service-to-port 
    //return addrinfo hints which is the only address with af_family = AF_UNSPEC
    //if service includes multiple socket type, return res base on the ai_socktype = SOCK_STREAM
    if ((n = getaddrinfo (host, port, &hints, &res)) != 0)
    {
        fprintf(stderr,"tcp_connect error for %s, %s: %s\n",host, port, gai_strerror (n));
        exit(1);
    }

    //assign ressave = res, ressave now is a linked list of addrinfo structures, 
    //linked through the ai_next pointer
    ressave = res;

    do {
        //create socket wich ai_family, ai_socktype, ai_protocol
        sockfd = socket (res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue;
        //if succeed create socket, try to establish connect, if connect succeed break loop
        if (connect (sockfd, res->ai_addr, res->ai_addrlen) == 0)
            break;
        close(sockfd);
    } while ( (res = res->ai_next) != NULL);


    if (res == NULL)
    {
        fprintf (stderr,"tcp_connect error for %s, %s", host, port);
        exit(1);
    }

    freeaddrinfo (ressave);

    return (sockfd);
}

int Tcp_connect (const char *host, const char *port)
{
    return (tcp_connect (host, port));
}

void getConnectionInfo(int sockfd)
{
    struct sockaddr_in clientAddr;
    socklen_t clientAddr_len = sizeof(clientAddr);

    if (getpeername(sockfd, (struct sockaddr *)&clientAddr, &clientAddr_len) == 0) 
    {
        char client_name[NI_MAXHOST];
        char client_ip[INET_ADDRSTRLEN];
        char client_port[NI_MAXHOST];
        if (getnameinfo((struct sockaddr*)&clientAddr, clientAddr_len, client_name, NI_MAXHOST, client_port,NI_MAXHOST, NI_NAMEREQD) == 0)
        {
            inet_ntop(AF_INET, &(clientAddr.sin_addr), client_ip, INET_ADDRSTRLEN);
            printf("Received request from client %s", client_name);
            //printf("\tClient IPAddress: %s\n", client_ip);
            printf(" port %s", client_port);
        }
        else
        {
            perror("getnameinfo error");
        }
    }
    else {
        perror("getpeername error");
    }
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

void getLocalInfo(int sockfd)
{
    struct sockaddr_storage local_addr;
    socklen_t addr_len = sizeof(local_addr);
    printf("Local information:\n");
    if (getsockname(sockfd, (struct sockaddr *)&local_addr, &addr_len) == 0)
    {
        char local_host[NI_MAXHOST], local_service[NI_MAXSERV];
        if (getnameinfo((struct sockaddr *)&local_addr, addr_len, local_host, sizeof(local_host), local_service, sizeof(local_service), NI_NUMERICHOST | NI_NUMERICSERV) == 0)
        {
            printf("\tLocal IP Address: %s\n", local_host);
            printf("\tLocal port number: %s\n\n", local_service);
        }
        else
        {
            perror("getnameinfo error for client");
        }
    }
    else
        perror("getsockname error");
}

//Get name of the connection that client is communicating to
char* getConnectionName(int sockfd)
{
    struct sockaddr_in serverAddr;
    socklen_t serverAddr_len = sizeof(serverAddr);
    char* server_name = (char*)malloc(NI_MAXHOST);
    if (getpeername(sockfd, (struct sockaddr *)&serverAddr, &serverAddr_len) == 0) 
    {
        if (getnameinfo((struct sockaddr*)&serverAddr, serverAddr_len, server_name, NI_MAXHOST, NULL, NI_MAXSERV, NI_NAMEREQD) == 0)
        {
            return server_name;
        }
        else
        {
            perror("getnameinfo error");
        }
    }
    free(server_name);
    return NULL;
}

int main(int argc, char **argv)
{
    int     listenfd, connfd, sockfd, n, m;
    struct sockaddr_in tunneladdr;
    struct message received_msg;
    struct message relay_message;

    if (argc != 2)
    {
        printf("usage: tunnel <Portnumber>\n");
    }

    long port_num = atoi(argv[1]);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&tunneladdr, sizeof(tunneladdr));
    tunneladdr.sin_family = AF_INET;
    tunneladdr.sin_addr.s_addr = htonl(INADDR_ANY);
    tunneladdr.sin_port = htons((uint16_t)port_num);

    if (bind(listenfd, (struct sockaddr *)&tunneladdr, sizeof(tunneladdr)) < 0)
    {
        perror("bind error");
        exit(1);
    }

    if (listen(listenfd, LISTENQ) < 0)
    {
        perror("listen error");
        exit(1);
    }

    printf("Tunnel started on port: %ld\n", port_num);

    for (;;)
    {
        connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
        if (connfd < 0)
        {
            perror("accept error");
            continue;
        }
        else
            printf("Accept connection.\n");

        getConnectionInfo(connfd);

        char server_addr[MAXLINE + 1];  
        char server_port[MAXLINE + 1];
        while ((n = read(connfd, &received_msg, sizeof(struct message))) > 0) {
            // Copy the address data from received_msg.addr to temp_addr
            memcpy(server_addr, received_msg.addr, received_msg.addrlen);
            // Null-terminate the string
            server_addr[received_msg.addrlen] = '\0';

            // Copy the address data from received_msg.addr to temp_addr
            memcpy(server_port, received_msg.payload, received_msg.msglen);
            // Null-terminate the string
            server_port[received_msg.msglen] = '\0';

            //printf("\tForward address: %s\n", server_addr);
            //printf("\tForward port: %s\n", server_port);
            break;
        }
        if (n < 0) {
            printf("read error\n");
            exit(1);
        }

        //printf("Connecting to server\n");
        sockfd = Tcp_connect(server_addr, server_port);
        //printf("Connected to server\n");
        

        char buffer[sizeof(struct message)];
        ssize_t totalBytesRead = 0;

        while ((m = read(sockfd, buffer + totalBytesRead, sizeof(struct message)-totalBytesRead)) > 0) {
            totalBytesRead += n;
             // Ensure null-termination of strings
            if (totalBytesRead == sizeof(struct message))
            {
                memcpy(&relay_message, buffer, sizeof(struct message));

                relay_message.addr[relay_message.addrlen] = '\0';
                relay_message.currtime[relay_message.timelen] = '\0';
                relay_message.payload[relay_message.msglen] = '\0';

                // printf("Received %d bytes\n", n);
                // printf("addrlen: %d\n", relay_message.addrlen);
                // printf("timelen: %d\n", relay_message.timelen);
                // printf("msglen: %d\n", relay_message.msglen);
                // printf("\tIP Address: %.*s\n", relay_message.addrlen, relay_message.addr);
                // printf("\tTime: %.*s", relay_message.timelen, relay_message.currtime);
                // printf("\tWho: %.*s\n", relay_message.msglen, relay_message.payload);

                totalBytesRead = 0;
                break;
            }

        }
        if (m < 0) {
            printf("read error\n");
            exit(1);
        }

        char* server_name = getConnectionName(sockfd);
        char ip_addr[MAXLINE + 1];  
        char time[MAXLINE + 1];

        memcpy(ip_addr, relay_message.addr, relay_message.addrlen);
        ip_addr[relay_message.addrlen] = '\0';
        memcpy(time, relay_message.payload, relay_message.msglen);
        ip_addr[relay_message.msglen] = '\0';
        
        relay_message.msglen = strlen(server_name);
        memcpy(relay_message.payload, server_name, relay_message.msglen);
        relay_message.payload[relay_message.msglen] = '\0';

        free(server_name); 
        
        struct message relay;

        close(sockfd);
        printf(" destined to server %.*s", relay_message.msglen, relay_message.payload);
        printf("port %s\n\n", server_port);
        //printf("\tIP Address: %.*s\n", relay_message.addrlen, relay_message.addr);
        //printf("\tTime: %.*s", relay_message.timelen, relay_message.currtime);
        fill_message(&relay, ip_addr, time, server_name); 
        write(connfd, &relay_message, sizeof(struct message));
        // printf("Relay msg sent!\n");
        close(connfd);
    }
}
