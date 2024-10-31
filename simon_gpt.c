#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

int server_socket, max_socket_fd, client_ids[1024], next_client_id = 0;
fd_set all_sockets_set, ready_sockets_set;
char message_buffer[100], single_char_buffer[2] = {};
struct sockaddr_in server_address = {0};

void err42(const char *s) {
    write(2, s, strlen(s));
    exit(1);
}

void send_msg(char *message, int sender_fd) {
    for (int fd = 0; fd <= max_socket_fd; fd++) {
        if (fd != sender_fd && FD_ISSET(fd, &all_sockets_set)) {
            send(fd, message, strlen(message), SO_NOSIGPIPE);
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2)
        err42("Wrong number of arguments\n");

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
        err42("Failed to create socket\n");

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_address.sin_port = htons(atoi(argv[1]));

    // Bind and listen on the socket
    if (bind(server_socket, (const struct sockaddr *)&server_address, sizeof(server_address)) != 0)
        err42("Bind failed\n");
    if (listen(server_socket, 128) != 0)
        err42("Listen failed\n");

    FD_ZERO(&all_sockets_set);
    FD_SET(server_socket, &all_sockets_set);
    max_socket_fd = server_socket;

    while (1) {
        ready_sockets_set = all_sockets_set;
        if (select(max_socket_fd + 1, &ready_sockets_set, NULL, NULL, NULL) < 0)
            continue;

        for (int socket_fd = 0; socket_fd <= max_socket_fd; socket_fd++) {
            if (!FD_ISSET(socket_fd, &ready_sockets_set))
                continue;

            if (socket_fd == server_socket) {  // New connection
                int client_socket = accept(server_socket, NULL, NULL);
                if (client_socket < 0)
                    continue;

                if (client_socket > max_socket_fd)
                    max_socket_fd = client_socket;
                
                client_ids[client_socket] = next_client_id++;
                FD_SET(client_socket, &all_sockets_set);
                sprintf(message_buffer, "server: client %d just arrived\n", client_ids[client_socket]);
                send_msg(message_buffer, client_socket);
            } else {  // Data from an existing client
                char input_buffer[200000];
                int bytes_received = recv(socket_fd, input_buffer, 200000, 0);
                if (bytes_received > 0) {
                    sprintf(message_buffer, "client %d: ", client_ids[socket_fd]);
                    send_msg(message_buffer, socket_fd);
                    for (int i = 0; i < bytes_received; i++) {
                        single_char_buffer[0] = input_buffer[i];
                        send_msg(single_char_buffer, socket_fd);
                        if (i + 1 < bytes_received && single_char_buffer[0] == '\n')
                            send_msg(message_buffer, socket_fd);
                    }
                    if (single_char_buffer[0] != '\n')
                        send_msg("\n", socket_fd);
                } else {  // Client disconnected
                    sprintf(message_buffer, "server: client %d just left\n", client_ids[socket_fd]);
                    send_msg(message_buffer, socket_fd);
                    FD_CLR(socket_fd, &all_sockets_set);
                    close(socket_fd);
                }
            }
        }
    }
}
