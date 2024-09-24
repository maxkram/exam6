#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>

const int BUFFER = 5000;

int sockfd, maxfd;
struct sockaddr_in servaddr;

fd_set currentfd, readfd;

int ids[FD_SETSIZE];
int client_counter = 0;

char server_buffer[BUFFER];
char client_buffer[BUFFER];

void error_message(char *msg) {
    write(2, msg, strlen(msg));
    exit(1);
}

void broadcast(int clientfd, char *buffer, int len) {
    for (int fd = 0; fd <= maxfd; fd++) {
        if (fd != clientfd) {
            send(fd, buffer, len, 0);
        }
    }
}

void send_message(int fd, int len) {
    sprintf(server_buffer, "client %d: ", ids[fd]);
    broadcast(fd, server_buffer, strlen(server_buffer));

    char send[2] = {};
    for (int i = 0; i < len; i++) {
        send[0] = client_buffer[i];
        broadcast(fd, send, strlen(send));

        if (i + 1 < len && send[1] == '\n') {
            broadcast(fd, server_buffer, strlen(server_buffer));
        }

        if (i + 1 == len && len == BUFFER) {
            len = recv(fd, client_buffer, BUFFER, 0);
            i = -1;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2)
        error_message("Wrong number of arguments\n");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        error_message("Fatal error\n");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));

    if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
         error_message("Fatal error\n");
    }

    if (listen(sockfd, 10) != 0) {
        error_message("Fatal error\n");
    }

    FD_ZERO(&currentfd);
    FD_ZERO(&readfd);
    FD_SET(sockfd, &currentfd);
    maxfd = sockfd;

    while (1) {
        readfd = currentfd;

        if ( select(maxfd + 1, &readfd, 0, 0, 0) < 0)
            continue;

        for (int fd = 0; fd <= maxfd; fd++) {
            if (!(FD_ISSET(fd, &readfd)))
                continue;
            if (sockfd == fd) {
                int clientfd = accept(sockfd, 0, 0);
                if (clientfd > maxfd)
                    maxfd = clientfd;
                ids[clientfd] = client_counter++;
                FD_SET(clientfd, &currentfd);
                sprintf(server_buffer, "server: client %d just arrived\n", ids[clientfd]);
                broadcast(clientfd, server_buffer, strlen(server_buffer));

            } else {
                int len = recv(fd, client_buffer, BUFFER, 0);
                if (len > 0) {
                    send_message(fd, len);
                } else {
                    sprintf(server_buffer, "server: client %d just left\n", ids[fd]);
                    broadcast(fd, server_buffer, strlen(server_buffer));
                    FD_CLR(fd, &currentfd);
                    close(fd);
                }
            }
        }
    }
}