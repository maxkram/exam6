#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

int serv_fd, max_fd, client_ids[1024], cnt = 0;
fd_set all_set, read_set;
char msg_buf[100], char_buf[2] = {};
struct sockaddr_in servaddr = {0};

void err42(const char *s) {
    write(2, s, strlen(s));
    exit(1);
}

void send_msg(char *msg, int sender_fd) {
    for (int fd = 0; fd <= max_fd; fd++) {
        if (fd != sender_fd && FD_ISSET(fd, &all_set)) {
            send(fd, msg, strlen(msg), SO_NOSIGPIPE);
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2)
        err42("Wrong number of arguments\n");

    if ((serv_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        err42("Fatal error\n");

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) != 0
        || listen(serv_fd, 128) != 0)
            err42("Fatal error\n");

    FD_ZERO(&all_set);
    FD_SET(serv_fd, &all_set);
    max_fd = serv_fd;

    while (1) {
        read_set = all_set;
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0)
            continue;

        for (int socket_fd = 0; socket_fd <= max_fd; socket_fd++) {
            if (!FD_ISSET(socket_fd, &read_set))
                continue;

            if (socket_fd == serv_fd) {
                int client_fd = accept(serv_fd, NULL, NULL);

                if (client_fd < 0)
                    continue;

                if (client_fd > max_fd)
                    max_fd = client_fd;

                client_ids[client_fd] = cnt++;
                FD_SET(client_fd, &all_set);
                sprintf(msg_buf, "server: client %d just arrived\n", client_ids[client_fd]);
                send_msg(msg_buf, client_fd);
            } else {
                char input_buf[200000];
                int bytes_recv = recv(socket_fd, input_buf, 200000, 0);

                if (bytes_recv > 0) {
                    sprintf(msg_buf, "client %d: ", client_ids[socket_fd]);
                    send_msg(msg_buf, socket_fd);

                    for (int i = 0; i < bytes_recv; i++) {
                        char_buf[0] = input_buf[i];
                        send_msg(char_buf, socket_fd);

                        if (i + 1 < bytes_recv && char_buf[0] == '\n')
                            send_msg(msg_buf, socket_fd);
                    }
                    if (char_buf[0] != '\n')
                        send_msg("\n", socket_fd);
                } else {
                    sprintf(msg_buf, "server: client %d just left\n", client_ids[socket_fd]);
                    send_msg(msg_buf, socket_fd);
                    FD_CLR(socket_fd, &all_set);
                    close(socket_fd);
                }
            }
        }
    }
}
