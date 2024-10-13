#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <arpa/inet.h>

int max_socket_fd;                  // Maximum socket descriptor
char *message = NULL;               // Pointer to the message to be broadcasted to clients
int client_ids[5000];               // Array to store unique client IDs associated with socket descriptors
char *client_buffers[5000];         // Array of pointers to store client message buffers
char send_buffer[1001];             // Buffer for sending data to clients
char receive_buffer[1001];          // Buffer for receiving data from clients
fd_set read_set, write_set, active_set; // Sets for monitoring file descriptors for reading, writing, and active sockets

// Function to handle fatal errors. Outputs an error message and exits the program.
void handle_fatal_error(const char *error_message) {
    perror(error_message);
    exit(1);     // Terminates the program with an error code
}

// Function to extract a complete message from a client's buffer.
int extract_message(char **buf, char **msg)
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

// Function to join two strings (buffers).
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

// Function to send a message to all connected clients.
void broadcast_message() {
    for (int socket_fd = 3; socket_fd <= max_socket_fd; socket_fd++) {
        if (FD_ISSET(socket_fd, &write_set)) {  // Check if the socket is ready for writing
            send(socket_fd, send_buffer, strlen(send_buffer), 0);  // Send the main buffer to the client
            if (message) 
                send(socket_fd, message, strlen(message), 0);  // Send the additional message if available
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2)  // Check if the correct number of arguments are provided (port number)
        handle_fatal_error("Wrong number of arguments\n");

    int server_socket_fd, client_socket_fd, client_id_counter = 0;  // Server socket, client socket, and client ID counter
    struct sockaddr_in server_address = {0}, client_address;  // Structures for server and client addresses
    socklen_t client_address_length = sizeof(client_address);  // Length of client address structure

    // Configure server address (127.0.0.1 and the provided port)
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_address.sin_port = htons(atoi(argv[1]));

    // Create the server socket, bind it, and start listening for connections
    if ((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
        bind(server_socket_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0 ||
        listen(server_socket_fd, SOMAXCONN) < 0)
            handle_fatal_error("Fatal error\n");

    FD_SET(max_socket_fd = server_socket_fd, &active_set);  // Add the server socket to the set of active sockets

    while (1) {
        read_set = write_set = active_set;  // Copy the active socket set for select
        if (select(max_socket_fd + 1, &read_set, &write_set, NULL, NULL) <= 0)
            continue;  // Wait for activity on any socket

        // Handle new connections
        if (FD_ISSET(server_socket_fd, &read_set)) {
            if ((client_socket_fd = accept(server_socket_fd, (struct sockaddr *) &client_address, &client_address_length)) < 0)
                handle_fatal_error("Fatal error\n");

            FD_SET(client_socket_fd, &active_set);  // Add the new client socket to the active set
            max_socket_fd = (client_socket_fd > max_socket_fd) ? client_socket_fd : max_socket_fd;  // Update the max socket FD
            client_ids[client_socket_fd] = client_id_counter++;  // Assign a unique client ID
            sprintf(send_buffer, "server: client %d just arrived\n", client_ids[client_socket_fd]);  // Format the new client message
            broadcast_message();  // Send the message to all clients
            client_buffers[client_socket_fd] = NULL;  // Initialize the new client's message buffer
        }

        // Handle data from connected clients
        for (int socket_fd = 3; socket_fd <= max_socket_fd; socket_fd++) {
            if (FD_ISSET(socket_fd, &read_set) && socket_fd != server_socket_fd) {  // If there's data from a client
                int bytes_received = recv(socket_fd, receive_buffer, 1000, 0);  // Receive data
                if (bytes_received <= 0) {  // If the client has disconnected
                    FD_CLR(socket_fd, &active_set);  // Remove the socket from the active set
                    sprintf(send_buffer, "server: client %d just left\n", client_ids[socket_fd]);  // Format the disconnect message
                    broadcast_message();  // Send the message to all clients
                    free(client_buffers[socket_fd]);  // Free the client's buffer
                    close(socket_fd);  // Close the client socket
                } else {  // If data is received
                    receive_buffer[bytes_received] = '\0';  // Null-terminate the string
                    client_buffers[socket_fd] = join_strings(client_buffers[socket_fd], receive_buffer);  // Add the received data to the client's buffer
                    while (extract_message(&client_buffers[socket_fd], &message)) {  // Extract complete messages
                        sprintf(send_buffer, "client %d: ", client_ids[socket_fd]);  // Format the message with the client ID
                        broadcast_message();  // Send the message to all clients
                        free(message);  // Free the memory allocated for the message
                    }
                }
            }
        }
    }
    return 0;
}
