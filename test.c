#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

int sockfd, maxfd;
struct sockaddr_in servaddr;

fd_set currfd, readfd;

int ids[FD_SETSIZE];
int cnt = 0;

char serv_buf[50];
char cli_buf[50];

void err42(char *s)
{
    write(2, s, strlen(s));
    exit(1);
}

void s_msg(int cli_fd, char *buf, int len)
{
    for (int fd = 0; fd <= maxfd; fd++)
        if (fd != cli_fd)
            send(fd, buf, len, 0);
}

