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
#include <errno.h>

#define MAXLINE     4096    /* max text line length */
#define DAYTIME_PORT 3333

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


//retrieve local information like ipaddress, portnumber
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

//Get port of the connection that client is communicating to
char* getConnectionPort(int sockfd)
{
    struct sockaddr_in serverAddr;
    socklen_t serverAddr_len = sizeof(serverAddr);
    char* server_port = (char*)malloc(INET_ADDRSTRLEN);
    if (getpeername(sockfd, (struct sockaddr *)&serverAddr, &serverAddr_len) == 0) 
    {
        if (getnameinfo((struct sockaddr*)&serverAddr, serverAddr_len, NULL, NI_MAXHOST, server_port,NI_MAXSERV, NI_NAMEREQD) == 0)
        {
            return server_port;
        }
        else
        {
            perror("getnameinfo error");
        }
    }
    free(server_port);
    return NULL;
}

//Get address of the connection that client is communicating to
char* getConnectionAddress(int sockfd)
{
    struct sockaddr_in serverAddr;
    socklen_t serverAddr_len = sizeof(serverAddr);
    char* server_ip = (char*)malloc(NI_MAXHOST);
    if (getpeername(sockfd, (struct sockaddr *)&serverAddr, &serverAddr_len) == 0) 
    {
        inet_ntop(AF_INET, &(serverAddr.sin_addr), server_ip, INET_ADDRSTRLEN);
        return server_ip;
    }
    free(server_ip);
    return NULL;
}

int main(int argc, char **argv)
{
    int     sockfd, n;
    struct message received_msg;
    struct message my_message;

    if (argc != 3 && argc != 5) 
    {
        fprintf(stderr, "Usage: client (<TunnelName/TunnelIPAddress> <TunnelPortNumber>) <ServerHostname/IPaddress> <PortNumber>\n");
        exit(1);
    }

    if (argc == 3)
    {
        const char *serverName = argv[1];
        const char *serverPort = argv[2];
        sockfd = Tcp_connect (serverName, serverPort);

        //getLocalInfo(sockfd);

        printf("\nServer Name: %s\n", getConnectionName(sockfd));
        free(getConnectionName(sockfd));
        // printf("%s\n", getConnectionPort(sockfd));
        // free(getConnectionPort(sockfd));
        // printf("%s\n", getConnectionAddress(sockfd));
        // free(getConnectionAddress(sockfd));

        char buffer[sizeof(struct message)];
        ssize_t totalBytesRead = 0;
        //read the server’s reply and display the result 
        while ((n = read(sockfd, buffer + totalBytesRead, sizeof(struct message)-totalBytesRead)) > 0) {
            totalBytesRead += n;

            if (totalBytesRead == sizeof(struct message))
            {
                memcpy(&received_msg, buffer, sizeof(struct message));

                received_msg.addr[received_msg.addrlen] = '\0';
                received_msg.currtime[received_msg.timelen] = '\0';
                received_msg.payload[received_msg.msglen] = '\0';

                //printf("Received %d bytes\n", n);
                printf("IP Address: %.*s\n", received_msg.addrlen, received_msg.addr);
                printf("Time: %.*s", received_msg.timelen, received_msg.currtime);
                printf("Who: %.*s\n", received_msg.msglen, received_msg.payload);

                totalBytesRead = 0;
            }
            
        }
        if (n < 0) {
            printf("read error\n");
            exit(1);
        }
    }

    if (argc == 5)
    {
        const char *tunnelName = argv[1];
        const char *tunnelPort = argv[2];
        const char *serverName = argv[3];
        const char *serverPort = argv[4];

        sockfd = Tcp_connect (tunnelName, tunnelPort);

        //initialize message to send to tunnel
        //printf("Preping message...\n");
        fill_message(&my_message, serverName, "", serverPort); //use payload to store port number
        //printf("\tmy_message.addr: %s\n", my_message.addr);
        //printf("\tmy_message.payload: %s\n", my_message.payload);
        //printf("...Finished\n");    
        write(sockfd, &my_message, sizeof(struct message));
        //printf("Message sent to tunnel.\n");
        //printf("Waiting response...\n");

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(atoi(serverPort));
        inet_pton(AF_INET, serverName, &(server_addr.sin_addr));

        //display message received from tunnel
        char buffer[sizeof(struct message)];
        ssize_t totalBytesRead = 0;

        while ((n = read(sockfd, buffer + totalBytesRead, sizeof(struct message) - totalBytesRead)) > 0) {
            totalBytesRead += n;

            // Check if the entire message has been received
             if (totalBytesRead == sizeof(struct message)) {
                // Copy the buffer into the received_msg structure
                memcpy(&received_msg, buffer, sizeof(struct message));

                // Ensure null-termination of strings
                received_msg.addr[received_msg.addrlen] = '\0';
                received_msg.currtime[received_msg.timelen] = '\0';
                received_msg.payload[received_msg.msglen] = '\0';

                // Print the entire message
                printf("Received %zd bytes\n", totalBytesRead);
                printf("\tServer Name: %.*s\n", received_msg.msglen, received_msg.payload);
                printf("\tIP Address: %.*s\n", received_msg.addrlen, received_msg.addr);
                printf("\tTime: %.*s", received_msg.timelen, received_msg.currtime);

                // Reset totalBytesRead for the next message
                totalBytesRead = 0;
            }
        }
        if (n < 0) {
            printf("read error\n");
            exit(1);
        }
        
        printf("\n");
        printf("\tVia Tunnel: %s\n", getConnectionName(sockfd));
        printf("\tIP Address: %s\n", getConnectionAddress(sockfd));
        printf("\tPort Number: %s\n\n", getConnectionPort(sockfd));

    }
    
    exit(0);
}

