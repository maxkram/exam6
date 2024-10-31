#include <stdio.h>      // Standard input/output library for functions like printf
#include <string.h>     // String manipulation functions like strlen, strcpy, strcat
#include <unistd.h>     // POSIX API for system calls like close, read, write, and select
#include <stdlib.h>     // General-purpose functions like malloc, free, exit
#include <sys/select.h> // select system call to monitor multiple file descriptors
#include <arpa/inet.h>  // Functions for dealing with IP addresses, like htons, htonl

int maxSock;                   // The largest file descriptor currently in use
char *msg = NULL;              // Pointer to store extracted message
int g_cliId[5000];      // Array storing client IDs for each socket
char *cliBuff[5000];    // Buffer array for each client to store incoming messages
char buff_sd[1001];     // Buffer used for sending data to clients
char buff_rd[1001];     // Buffer used for reading data from clients
fd_set rd_set, wrt_set, atv_set; // Sets of file descriptors for reading, writing, and active descriptors

void ft_error(const char *s) {
    write(2, s, strlen(s));        // Print the error message
    exit(1);          // Exit the program with an error code
}

int extract_message(char **buf, char **msg) {
    char *newbuf;
    int i;

    *msg = 0;                // Initialize message to NULL
    if (*buf == 0)           // If buffer is NULL, return 0
        return (0);
    i = 0;
    while ((*buf)[i]) {      // Loop through the buffer to find a newline character
        if ((*buf)[i] == '\n') {
            newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));  // Allocate space for the rest of the buffer after '\n'
            if (newbuf == 0)
                return (-1); // Return -1 on memory allocation failure
            strcpy(newbuf, *buf + i + 1);  // Copy remaining buffer to newbuf
            *msg = *buf;     // Assign the original buffer to msg
            (*msg)[i + 1] = 0; // Null-terminate the message
            *buf = newbuf;   // Update the buffer pointer
            return (1);      // Return 1 to indicate a message was extracted
        }
        i++;
    }
    return (0);              // No message extracted
}

char *str_join(char *buf, char *add) {
    char *newbuf;
    int len;

    if (buf == 0)            // If the existing buffer is NULL, length is 0
        len = 0;
    else
        len = strlen(buf);    // Otherwise, get the length of the existing buffer
    newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1)); // Allocate memory for the new buffer
    if (newbuf == 0)
        return (0);           // Return 0 on memory allocation failure
    newbuf[0] = 0;
    if (buf != 0)             // If buffer exists, concatenate it
        strcat(newbuf, buf);
    free(buf);                // Free the old buffer
    strcat(newbuf, add);      // Append the new data
    return (newbuf);          // Return the combined buffer
}

void send_msg() {
    for (int sockId = 3; sockId <= maxSock; sockId++) {  // Iterate through all sockets
        if (FD_ISSET(sockId, &wrt_set)) {             // If the socket is ready for writing
            send(sockId, buff_sd, strlen(buff_sd), 0);   // Send server buffer to the client
            if (msg)                                    // If there's an additional message
                send(sockId, msg, strlen(msg), 0);      // Send it to the client
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2)                                  // Check if the program is run with the correct number of arguments
        ft_error("Wrong number of arguments\n");
    int sockfd, connfd, cliId = 0;                  // Declare variables for the server and client socket
    struct sockaddr_in servaddr = {0}, cliaddr;     // Server and client address structures
    socklen_t len_cli = sizeof(cliaddr);            // Size of the client address structure

    servaddr.sin_family = AF_INET;                  // Set the server address family to IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Bind to localhost (loopback address)
    servaddr.sin_port = htons(atoi(argv[1]));       // Set the port to the one provided in the arguments
  
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||   // Create a socket
        bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 || // Bind the socket to the address
        listen(sockfd, SOMAXCONN) < 0)                 // Start listening for connections
        ft_error("Fatal error\n");                     // Handle errors

    FD_ZERO(&atv_set);
    FD_SET(maxSock = sockfd, &atv_set);                // Add the server socket to the active set

    while (1) {                                        // Main server loop
        rd_set = wrt_set = atv_set;                    // Reset read and write sets to the active set
        if (select(maxSock + 1, &rd_set, &wrt_set, NULL, NULL) <= 0)  // Wait for activity on any socket
            continue;                                  // If no activity, continue to the next iteration
        if (FD_ISSET(sockfd, &rd_set)) {               // If the server socket is ready to read (new connection)
            if ((connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len_cli)) < 0)  // Accept new client connection
                ft_error("Fatal error\n");
            FD_SET(connfd, &atv_set);                  // Add new client to the active set
            maxSock = (connfd > maxSock) ? connfd : maxSock; // Update the maximum socket descriptor
            g_cliId[connfd] = cliId++;                 // Assign a unique ID to the client
            sprintf(buff_sd, "server: client %d just arrived\n", g_cliId[connfd]);  // Notify all clients of the new connection
            send_msg();
            cliBuff[connfd] = NULL;                    // Initialize client's buffer to NULL
        }
        for (int sockId = 3; sockId <= maxSock; sockId++) {   // Iterate over all possible sockets
            if (FD_ISSET(sockId, &rd_set) && sockId != sockfd) {  // If a client socket has data to read
                int rd = recv(sockId, buff_rd, 1000, 0); // Read data from the client
                if (rd <= 0) {                            // If no data is received or error
                    FD_CLR(sockId, &atv_set);             // Remove the client from the active set
                    sprintf(buff_sd, "server: client %d just left\n", g_cliId[sockId]); // Notify all clients of the disconnection
                    send_msg();
                    free(cliBuff[sockId]);                // Free the client's buffer
                    close(sockId);                        // Close the client socket
                } else {
                    buff_rd[rd] = '\0';                   // Null-terminate the received data
                    cliBuff[sockId] = str_join(cliBuff[sockId], buff_rd); // Append the received data to the client's buffer
                    while (extract_message(&cliBuff[sockId], &msg)) { // Extract any complete messages
                           // Prefix the message with the client ID
                        send_msg();                 // Send the message to all clients
                        free(msg);                        // Free the extracted message
                    }
                }
            }
        }
    }
    return 0;
}
