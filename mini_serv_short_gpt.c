#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <stdio.h>

int max_fd;
fd_set active_set, read_set, write_set;
int client_ids[1024];
char *msg_buf[1024];
char send_buffer[4096], recv_buffer[4096];

void err42(const char *s) {
    write(2, s, strlen(s));
    exit(1);
}

int main(int argc, char **argv) {
    if (argc != 2)
        err42("Wrong number of arguments\n");

    int server_fd, client_fd, port = atoi(argv[1]);
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err42("Fatal error\n");

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(server_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ||
        listen(server_fd, SOMAXCONN) < 0)
        err42("Fatal error\n");

    FD_ZERO(&active_set);
    FD_SET(server_fd, &active_set);
    max_fd = server_fd;

    int cli_id = 0;
    while (1) {
        read_set = write_set = active_set;
        if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0)
            continue;

        if (FD_ISSET(server_fd, &read_set)) {
            if ((client_fd = accept(server_fd, (struct sockaddr *)&cliaddr, &len)) < 0)
                continue;
            client_ids[client_fd] = cli_id++;
            FD_SET(client_fd, &active_set);
            max_fd = (client_fd > max_fd) ? client_fd : max_fd;
            sprintf(send_buffer, "server: client %d just arrived\n", client_ids[client_fd]);
            for (int i = 0; i <= max_fd; i++)
                if (FD_ISSET(i, &write_set) && i != server_fd)
                    send(i, send_buffer, strlen(send_buffer), 0);
        }

        for (int fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, &read_set) && fd != server_fd) {
                int rcv = recv(fd, recv_buffer, 4096, 0);
                if (rcv <= 0) {
                    sprintf(send_buffer, "server: client %d just left\n", client_ids[fd]);
                    for (int i = 0; i <= max_fd; i++)
                        if (FD_ISSET(i, &write_set) && i != server_fd)
                            send(i, send_buffer, strlen(send_buffer), 0);
                    FD_CLR(fd, &active_set);
                    close(fd);
                } else {
                    recv_buffer[rcv] = '\0';
                    sprintf(send_buffer, "client %d: %s", client_ids[fd], recv_buffer);
                    for (int i = 0; i <= max_fd; i++)
                        if (FD_ISSET(i, &write_set) && i != fd && i != server_fd)
                            send(i, send_buffer, strlen(send_buffer), 0);
                }
            }
        }
    }
    return 0;
}
