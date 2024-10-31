#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

int sockfd;
struct sockaddr_in servaddr = {0};
int max;
int ids[FD_SETSIZE];
int maxId = 0;
fd_set active, ready;
char msg[100] = {};
char sending[2] = {};

void writeError(char* message){
	write(2, message, strlen(message));
	exit(1);
}

void sendAll(int id, char* message){
	for (int fd = 0; fd < max + 1; fd++)
		if (fd != id)
			send(fd, message, strlen(message), SO_NOSIGPIPE);
}

int main(int argc, char** argv) {
	if (argc != 2)
		writeError("Wrong number of arguments\n");
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		writeError("Fatal error\n");
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(atoi(argv[1]));
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0 || listen(sockfd, SOMAXCONN) != 0)
		writeError("Fatal error\n");
	FD_ZERO(&active);
	FD_SET(sockfd, &active);
	max = sockfd;
	while (1){
		ready = active;
		if (select(max + 1, &ready, NULL, NULL, NULL) < 0) 
			continue;
		for (int id = 0; id < max + 1; id++){
			if (!(FD_ISSET(id, &ready)))
				continue;
			if (id == sockfd){
				int client = accept(sockfd, NULL, NULL);
				if (client < 0)
					continue;
				max = client > max ? client : max;
				ids[client] = maxId++;
				FD_SET(client, &active);
				sprintf(msg, "server: client %d just arrived\n", ids[client]);
				sendAll(client, msg);
			} else {
				char buffer[200000];
				int read = recv(id, buffer, 200000, 0);
				if (read > 0){
					sprintf(msg, "client %d: ", ids[id]);
					sendAll(id, msg);
					for (int i = 0; i < read; i++){
						sending[0] = buffer[i];
						sendAll(id, sending);
						if (i + 1 < read && sending[0] == '\n')
							sendAll(id, msg);
					}
					if (sending[0] != '\n')
						sendAll(id, "\n");
				} else {
					sprintf(msg, "server: client %d just left\n", ids[id]);
					sendAll(id, msg);
					FD_CLR(id, &active);
					close(id);
				}
			}
		}
	}
}