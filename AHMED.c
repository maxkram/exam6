#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

int server, client, fd_max;
struct sockaddr_in servaddr, cli;
socklen_t len = sizeof(cli);

typedef struct s_client
{
    int id;
    char *msg;
} t_client;

t_client clients[1024];

char recv_buffer[1024], send_buffer[1024];
int limit = 0;

fd_set fd_read, fd_write, fd_master;

void err_msg(char *s)
{
    write(2, s, strlen(s));
    exit(1);
}

int extract_message(char **buf, char **msg)
{
    char *newbuf;
    int i;

    *msg = 0;
    if (*buf == 0)
        return (0);
    i = 0;
    while ((*buf)[i])
    {
        if ((*buf)[i] == '\n')
        {
            newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
            if (newbuf == 0)
                err_msg("Fatal error\n");
            strcpy(newbuf, *buf + i + 1);
            *msg = *buf;
            (*msg)[i + 1] = 0;
            *buf = newbuf;
            return (1);
        }
        i++;
    }
    return (0);
}

char *str_join(char *buf, char *add)
{
    char *newbuf;
    int len;

    if (buf == 0)
        len = 0;
    else
        len = strlen(buf);
    newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
    if (newbuf == 0)
        err_msg("Fatal error\n");
    newbuf[0] = 0;
    if (buf != 0)
        strcat(newbuf, buf);
    free(buf);
    strcat(newbuf, add);
    return (newbuf);
}

void send_all(int sender, char *msg)
{
    for (int fd = 0; fd <= fd_max; fd++)
        if (fd != sender && FD_ISSET(fd, &fd_write))
            send(fd, msg, strlen(msg), 0);
}

int main(int ac, char **av)
{
    if (ac != 2)
        err_msg("Wrong number of arguments\n");

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
        err_msg("Fatal error\n");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
    servaddr.sin_port = htons(atoi(av[1]));
    if ((bind(server, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        err_msg("Fatal error\n");
    if (listen(server, 0) != 0)
        err_msg("Fatal error\n");
    FD_ZERO(&fd_master);
    FD_SET(server, &fd_master);
    fd_max = server;
    while (1)
    {
        fd_read = fd_write = fd_master;
        if (select(fd_max + 1, &fd_read, &fd_write, NULL, NULL) < 0)
            continue;
        for (int fd = 0; fd <= fd_max; fd++)
        {
            if (FD_ISSET(fd, &fd_read))
            {
                if (fd == server)
                {
                    client = accept(server, (struct sockaddr *)&cli, &len);
                    if (client >= 0)
                    {
                        if (client > fd_max)
                            fd_max = client;
                        clients[client].id = limit++;
                        clients[client].msg = NULL;
                        sprintf(send_buffer, "server: client %d just arrived\n", clients[client].id);
                        send_all(client, send_buffer);
                        FD_SET(client, &fd_master);
                        break;
                    }
                }
                else
                {
                    int byte = recv(fd, recv_buffer, 1023, 0);
                    if (byte <= 0)
                    {
                        sprintf(send_buffer, "server: client %d just left\n", clients[fd].id);
                        send_all(fd, send_buffer);
                        free(clients[fd].msg);
                        FD_CLR(fd, &fd_master);
                        close(fd);
                        break;
                    }
                    recv_buffer[byte] = 0;
                    clients[fd].msg = str_join(clients[fd].msg, recv_buffer);
                    char *msg = NULL;
                    while (extract_message(&(clients[fd].msg), &msg))
                    {
                        sprintf(send_buffer, "client %d: ", clients[fd].id);
                        send_all(fd, send_buffer);
                        send_all(fd, msg);
                        free(msg);
                    }
                }
            }
        }
    }
    return 0;
}
