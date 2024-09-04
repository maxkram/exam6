#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
int sockfd, maxfd, ids[1024], maxid = 0;
fd_set perma, active;
char msg[100], sending[2] = {};
struct sockaddr_in servaddr = {0};
void sendError(char *msgs){
    write(2, msgs, strlen(msgs));
    exit(1);
}
void sendAll(char * msgs, int id) {
    for (int fd = 0; fd < maxfd +1; fd ++){
        if (fd != id){
            send(fd, msgs, strlen(msgs), SO_NOSIGPIPE);
        }
    }
}
int main(int argc, char **argv){
    if (argc != 2)
        sendError("Wrong number of arguments\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        sendError("Fatal error\n");
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
    servaddr.sin_port = htons(atoi(argv[1]));
    if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        sendError("Fatal error\n");
    if (listen(sockfd, 128) != 0)
        sendError("Fatal error\n");
    FD_ZERO(&perma);
    FD_SET(sockfd, &perma);
    maxfd = sockfd;
    while(1){
        active = perma;
        if (select(maxfd+1, &active, NULL, NULL, NULL) < 0)
            continue;
        for (int id = 0; id < maxfd+1; id++){
            if (!FD_ISSET(id, &active))
                continue;
            if (id == sockfd){
                int client = accept(sockfd, NULL, NULL);
                if (client < 0)
                    continue;
                if (client > maxfd)
                    maxfd = client;
                ids[client] = maxid++;
                FD_SET(client, &perma);
                sprintf(msg, "server: client %d just arrived\n", ids[client]);
                sendAll(msg, client);
            }
            else {
                char buffer[200000];
                int read = recv(id, buffer, 200000, 0);
                if (read > 0){
                    sprintf(msg, "client %d: ", ids[id]);
                    sendAll(msg, id);
                    for (int i = 0; i < read; i++){
                        sending[0] = buffer[i];
                        sendAll(sending, id);
                        if (1 +i < read && sending[0] == '\n')
                            sendAll(msg, id);
                    }
                    if (sending[0] != '\n')
                        sendAll("\n", id);
                }
                else{
                    sprintf(msg, "server: client %d just left\n", ids[id]);
                    sendAll(msg, id);
                    FD_CLR(id, &perma);
                    close(id);
                }
            }
        }
    }
}