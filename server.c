#include <netdb.h>
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

void getConnectionInfo(int sockfd)
{
    struct sockaddr_in clientAddr;
    socklen_t clientAddr_len = sizeof(clientAddr);

    if (getpeername(sockfd, (struct sockaddr *)&clientAddr, &clientAddr_len) == 0) 
    {
        char client_name[NI_MAXHOST];
        char client_ip[INET_ADDRSTRLEN];
        char client_port[NI_MAXHOST];
        printf("Client information:\n");
        if (getnameinfo((struct sockaddr*)&clientAddr, clientAddr_len, client_name, NI_MAXHOST, client_port,NI_MAXHOST, NI_NAMEREQD) == 0)
        {
            inet_ntop(AF_INET, &(clientAddr.sin_addr), client_ip, INET_ADDRSTRLEN);
            printf("\tClient Name: %s\n", client_name);
            printf("\tClient IPAddress: %s\n", client_ip);
            printf("\tClient port: %s\n", client_port);
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

char* getLocalAddress(int sockfd, struct sockaddr_in servaddr)
{
    char* server_ip = (char*)malloc(INET_ADDRSTRLEN); //retrieve server ip address
    socklen_t addr_len = sizeof(servaddr);
    //obtain the local address through getsockname
    if (getsockname(sockfd, (struct sockaddr *)&servaddr, &addr_len) == 0) 
    {
        if (inet_ntop(AF_INET, &(servaddr.sin_addr), server_ip, INET_ADDRSTRLEN) == NULL) 
        {
            perror("inet_ntop error");
            free(server_ip);
            return NULL;
        }
        return server_ip;
    }
    else {
        perror("getsockname error");
    }

    free(server_ip);
    return NULL;
}

int main(int argc, char **argv)
{
    int     listenfd, connfd;
    struct sockaddr_in servaddr;
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

    printf("Start listening on port: %ld \n",port_num);

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

        getConnectionInfo(connfd);
        
        char* server_ip = getLocalAddress(connfd, servaddr);
        ticks = time(NULL);
        //appended current time to the string buff
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks)); //ctime() ticks value into a human-readable
        

        //initilize message
        // read the output of the who command into the payload
        FILE *who_process = popen("who", "r");
        if (fread(my_message.payload, 1, sizeof(my_message.payload), who_process) > 0) {
            printf("\n");
        } else {
            // Handle the case where fread returns an error or reaches the end of the file
            perror("fread error");
        }
        pclose(who_process);

        // printf("Preping message...\n");
        fill_message(&my_message, server_ip, buff, my_message.payload);
        // printf("\tIP Address: %s\n", my_message.addr);
        // printf("\tTime: %s", my_message.currtime);
        // printf("\tWho: %s", my_message.payload);
        // printf("...Finished\n");        

        //the message is copied to connfd and send back to client by write()
        //printf("Sending message to client...\n");
        write(connfd, &my_message, sizeof(struct message));
        //printf("Message sent!\n");
        

        //Terminate connection
        close(connfd);
        //printf("Connection closed.\n\n");
    }
    
}

