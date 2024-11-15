#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

int server, maxfd, ids[FD_SETSIZE], cnt = 0;
struct sockaddr_in servaddr;
fd_set act_fd, readfd;
char serv_buf[50], cli_buf[50];

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

void send_msg(int fd, int len)
{
	sprintf(serv_buf, "client %d: ", ids[fd]);
	s_msg(fd, serv_buf, strlen(serv_buf));
	char send[2] = {};
	for (int i = 0; i < len; i++)
	{
		send[0] = cli_buf[i];
		s_msg(fd, send, strlen(send));
		if (i + 1 < len && send[i] == '\n')
			s_msg(fd, serv_buf, strlen(serv_buf));
		if (i + 1 == len && len == 50)
		{
			len = recv(fd, cli_buf, 50, 0);
			i = -1;
		}
	}
}

int main(int ac, char **av)
{
	if (ac != 2)
		err42("Wrong number of arguments\n");
	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == -1)
		err42("Fatal error\n");
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(atoi(av[1]));
	if ((bind(server, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		err42("Fatal error\n");
	if (listen(server, 10) != 0)
		err42("Fatal error\n");
	FD_ZERO(&act_fd);
	FD_ZERO(&readfd);
	FD_SET(server, &act_fd);
	maxfd = server;
	while (1)
	{
		readfd = act_fd;
		if (select(maxfd + 1, &readfd, 0, 0, 0) < 0)
			continue;
		for (int fd = 0; fd <= maxfd; fd++)
		{
			if (!(FD_ISSET(fd, &readfd)))
				continue;
			if (server == fd)
			{
				int client = accept(server, 0, 0);
				if (client > maxfd)
					maxfd = client;
				ids[client] = cnt++;
				FD_SET(client, &act_fd);
				sprintf(serv_buf, "server: client %d just arrived\n", ids[client]);
				s_msg(client, serv_buf, strlen(serv_buf));
			}
			else
			{
				int len = recv(fd, cli_buf, 50, 0);
				if (len > 0)
					send_msg(fd, len);
				else
				{
					sprintf(serv_buf, "server: client %d just left\n", ids[fd]);
					s_msg(fd, serv_buf, strlen(serv_buf));
					FD_CLR(fd, &act_fd);
					close(fd);
				}
			}
		}
	}
}