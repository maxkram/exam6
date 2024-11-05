#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
int sockfd, maxfd;
struct sockaddr_in servaddr;
fd_set currfd, readfd;
int ids[FD_SETSIZE];
int cnt = 0;
char serv_buf[50];
char cli_buf[50];
void err42(char *s) {
    write(2, s, strlen(s));
    exit(1);
}
void s_msg(int clifd, char *buf, int len) {
    for (int fd = 0; fd <= maxfd; fd++)
        if (fd != clifd)
            send(fd, buf, len, 0);
}
void send_msg(int fd, int len) {
    sprintf(serv_buf, "client %d: ", ids[fd]);
    s_msg(fd, serv_buf, strlen(serv_buf));
    char send[2] = {};
    for (int i = 0; i < len; i++) {
        send[0] = cli_buf[i];
        s_msg(fd, send, strlen(send));
        if (i + 1 < len && send[1] == '\n')
            s_msg(fd, serv_buf, strlen(serv_buf));
        if (i + 1 == len && len == 50) {
            len = recv(fd, cli_buf, 50, 0);
            i = -1;
        }
    }
}
int main(int ac, char **av) {
    if (ac != 2)
        err42("Wrong number of arguments\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        err42("Fatal error\n");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));    
    if ((bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr))) != 0)
        err42("Fatal error\n");
    if (listen(sockfd, 10) != 0)
        err42("Fatal error\n");
    FD_ZERO(&currfd);
    FD_ZERO(&readfd);
    FD_SET(sockfd, &currfd);
    maxfd = sockfd;
    while(1) {
        readfd = currfd;
        if (select(maxfd + 1, &readfd, 0, 0, 0) < 0)
            continue;
        for (int fd = 0; fd <= maxfd; fd++) {
            if (!(FD_ISSET(fd, &readfd)))
                continue;
            if (sockfd == fd) {
                int clientfd = accept(sockfd, 0, 0);
                if (clientfd > maxfd)
                    maxfd = clientfd;
                ids[clientfd] = cnt++;
                FD_SET(clientfd, &currfd);
                sprintf(serv_buf, "server: client %d just arrived\n", ids[clientfd]);
                s_msg(clientfd, serv_buf, strlen(serv_buf));
            } else {
                int len = recv(fd, cli_buf, 50, 0);
                if (len > 0) {
                    send_msg(fd, len);
                } else {
                    sprintf(serv_buf, "server: client %d just arrived\n", ids[fd]);
                    s_msg(fd, serv_buf, strlen(serv_buf));
                    FD_CLR(fd, &currfd);
                    close(fd);
                }
            }
        }
    }

}