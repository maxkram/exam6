#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <arpa/inet.h>

int max_socket_fd;
char *msg = NULL;
int client_ids[5000];
char *cli_buf[5000];
char send_buffer[1001];
char recv_buf[1001];
fd_set read_set, write_set, active_set;

void err42(const char *msgs) {
    write(2, msgs, strlen(msgs));
    exit(1);
}

int extract_msg(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

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
				return (-1);
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
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void send_msg() {
    for (int socket_fd = 3; socket_fd <= max_socket_fd; socket_fd++) {
        if (FD_ISSET(socket_fd, &write_set)) {
            send(socket_fd, send_buffer, strlen(send_buffer), 0);
            if (msg) 
                send(socket_fd, msg, strlen(msg), 0);
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2)
        err42("Wrong number of arguments\n");

    int server_fd, client_fd, cli_id = 0;
    struct sockaddr_in servaddr = {0}, cliaddr;
    socklen_t cliaddr_len = sizeof(cliaddr);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    servaddr.sin_port = htons(atoi(argv[1]));

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
        bind(server_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ||
        listen(server_fd, SOMAXCONN) < 0)
            err42("Fatal error\n");
    FD_ZERO(&active_set);
    FD_SET(max_socket_fd = server_fd, &active_set);

    while (1) {
        read_set = write_set = active_set;
        if (select(max_socket_fd + 1, &read_set, &write_set, NULL, NULL) <= 0)
            continue;

        if (FD_ISSET(server_fd, &read_set)) {
            if ((client_fd = accept(server_fd, (struct sockaddr *) &cliaddr, &cliaddr_len)) < 0)
                err42("Fatal error\n");

            FD_SET(client_fd, &active_set);
            max_socket_fd = (client_fd > max_socket_fd) ? client_fd : max_socket_fd;
            client_ids[client_fd] = cli_id++;
            sprintf(send_buffer, "server: client %d just arrived\n", client_ids[client_fd]);
            send_msg();
            cli_buf[client_fd] = NULL;
        }

        for (int socket_fd = 3; socket_fd <= max_socket_fd; socket_fd++) {
            if (FD_ISSET(socket_fd, &read_set) && socket_fd != server_fd) {
                int byt_rcv = recv(socket_fd, recv_buf, 1000, 0);
                if (byt_rcv <= 0) {
                    FD_CLR(socket_fd, &active_set);
                    sprintf(send_buffer, "server: client %d just left\n", client_ids[socket_fd]);
                    send_msg();
                    free(cli_buf[socket_fd]);
                    close(socket_fd);
                } else {
                    recv_buf[byt_rcv] = '\0';
                    cli_buf[socket_fd] = str_join(cli_buf[socket_fd], recv_buf);
                    while (extract_msg(&cli_buf[socket_fd], &msg)) {
                        sprintf(send_buffer, "client %d: ", client_ids[socket_fd]);
                        send_msg();
                        free(msg);
                    }
                }
            }
        }
    }
    return 0;
}
