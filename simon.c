#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

    int sockfd, maxfd;
    struct sockaddr_in servaddr;
    fd_set readfd, currfd;
    int indexe[FD_SETSIZE];
    int cli_num = 0;
    int BUFF = 200000;
    char serv_buf[200000];
    char cli_buf[200000];

void err42(char *s) {
    write (2, s, strlen(s));
    exit(1);
}
void broadcast(int clientfd, char* msg, int len)
{
    int fd = 0;
    while (fd <= maxfd) {
        if (clientfd != fd && FD_ISSET(fd, &currfd))
            send(fd, msg,len,SO_NOSIGPIPE);
        fd++;
    }
}
void send_msg(int client, int len) {
    bzero(serv_buf, strlen(serv_buf));
    sprintf(serv_buf, "client %d: ", indexe[client]);
    broadcast(client, serv_buf, strlen(serv_buf));
    char send;
    int i = 0;
    while (i < len) {
        send = cli_buf[i];
        broadcast(client, &send, 1);
        if (i + 1 < len && send == '\n')
            broadcast(client, serv_buf, strlen(serv_buf));
        if (i + 1 == len && len == BUFF) {
            bzero(cli_buf,strlen(cli_buf));
            len = recv(client, cli_buf, BUFF, 0);
            i = -1;
        }
        i++;
    }
}
int main(int ac, char **av) {
     if (ac != 2)
        err42("Wrong number of arguments\n");
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        err42("Fatal error\n");
    }
    bzero(&servaddr, sizeof(servaddr));
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
    servaddr.sin_port = htons(atoi(av[1]));      ////changer ici pour atoi(av[1])
    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) ||
        listen(sockfd, 10) != 0)
            err42("Fatal error\n");
    FD_ZERO(&readfd);
    FD_ZERO(&currfd);
    FD_SET(sockfd, &currfd);
    maxfd = sockfd;
    while(1) {
        readfd = currfd;
        if (select(maxfd + 1, &readfd,0,0,0) < 0)
            continue;
        for(int fd = 0 ; fd <= maxfd; fd++) {
            if (!(FD_ISSET(fd, &readfd)))
                continue;
            if (sockfd == fd ) {
                int clientfd = accept(sockfd,0,0);
                if (clientfd > maxfd)
                    maxfd = clientfd;
                indexe[clientfd] = cli_num++;
                FD_SET(clientfd, &currfd);
                sprintf(serv_buf, "server: client %d just arrived\n", indexe[clientfd] );
                broadcast(clientfd, serv_buf, strlen(serv_buf));
            }
            else {
                int len = recv(fd, cli_buf, 200000, 0);
                if (len > 0) {
                    send_msg(fd,len);
                }
                else {
                    sprintf(serv_buf,"server: client %d just left\n", indexe[fd]);
                    broadcast(fd, serv_buf, strlen(serv_buf));
                    FD_CLR(fd, &currfd);
                    close(fd);
                }
            }
        }
    }
}