#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>

// #define MAX_CLIENTS 128
// #define BUFFER_SIZE 200000

typedef struct s_client
{
	int fd;
	int id;
} t_client;

void	exit_error_message(char *str)
{
	write(2, &str, strlen(str));
	exit(1);
}

void	msg_clients(char *str, t_client *clients, int fd)
{
	const int MAX_CLIENTS = 128;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (clients[i].fd != 0 && clients[i].fd != fd)
			send(clients[i].fd, str, strlen(str), 0);
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2)
		exit_error_message("Wrong number of arguments\n");

	const int BUFFER_SIZE = 200000;
	const int MAX_CLIENTS = 128;
	t_client clients[MAX_CLIENTS];
	int next_id = 0;
	int server_socket;
	fd_set active_sockets, ready_sockets;
	char buffer[BUFFER_SIZE];
	char msg_buffer[BUFFER_SIZE];
	char sub_buffer[BUFFER_SIZE];

	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		exit_error_message("Fatal error\n");
	
	struct sockaddr_in server_address = {0};
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	server_address.sin_port = htons(atoi(argv[1]));

	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
		exit_error_message("Fatal error\n");

	if (listen(server_socket , MAX_CLIENTS) < 0)
		exit_error_message("Fatal error\n");

	bzero(clients, sizeof(t_client) * MAX_CLIENTS);
	FD_ZERO(&active_sockets);
	FD_SET(server_socket, &active_sockets);
	int max_socket = server_socket;

	while (1)
	{
		ready_sockets = active_sockets;
		if (select(max_socket + 1, &ready_sockets, NULL, NULL, NULL) < 0) 
			exit_error_message("Fatal error\n");
		for (int socket_id = 0; socket_id <= max_socket; socket_id++) 
		{
			if (!FD_ISSET(socket_id, &ready_sockets))
				continue ;
			bzero(buffer, BUFFER_SIZE);
			if (socket_id == server_socket)
			{
				int client_socket;
				if ((client_socket = accept(server_socket, NULL, NULL)) < 0)
					exit_error_message("Fatal error\n");
				FD_SET(client_socket, &active_sockets);
				if (client_socket > max_socket)
					max_socket = client_socket;
				clients[client_socket].fd = client_socket;
				clients[client_socket].id = next_id++;
				sprintf(buffer, "server: client %d just arrived\n", clients[client_socket].id);
				msg_clients(buffer, clients, client_socket);
			}
			else
			{
				int bytes_read = recv(socket_id, buffer, sizeof(buffer) - 1, 0);

				if (bytes_read <= 0) 
				{
					bzero(msg_buffer, BUFFER_SIZE);
					sprintf(msg_buffer, "server: client %d just left\n", clients[socket_id].id);
					msg_clients(msg_buffer, clients, socket_id);
					close(socket_id);
					FD_CLR(socket_id, &active_sockets);
				}
				else
				{
					for (int i = 0, j = strlen(sub_buffer); i < bytes_read; i++, j++) {
						sub_buffer[j] = buffer[i];
						if (sub_buffer[j] == '\n')
						{
							sub_buffer[j] = '\0';
							bzero(msg_buffer, BUFFER_SIZE);
							sprintf(msg_buffer, "client %d: %s\n", clients[socket_id].id, sub_buffer);
							
							msg_clients(msg_buffer, clients, socket_id);
							bzero(sub_buffer, strlen(sub_buffer));
							j = -1;
						}
					}
				}
				
			}
		}
	}
}