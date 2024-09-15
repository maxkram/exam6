#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAX_CLIENTS 5000
#define BUFFER_SIZE 1001

// Объявляются глобальные переменные:

// maxSock: максимальный дескриптор сокета
// msg: указатель на сообщение
// g_cliId: массив идентификаторов клиентов
// cliBuff: массив буферов клиентов
// buff_sd, buff_rd: буферы для отправки и чтения
// rd_set, wrt_set, atv_set: наборы файловых дескрипторов для select()

int maxSock;
char *msg = NULL;
int g_cliId[MAX_CLIENTS];
char *cliBuff[MAX_CLIENTS];
char buff_sd[BUFFER_SIZE];
char buff_rd[BUFFER_SIZE];
fd_set rd_set, wrt_set, atv_set;

void ft_error(const char *s) {
    perror(s);
    exit(1);
}

// Функция для объединения двух строк.

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

// Функция для извлечения сообщения из буфера

int extract_msg(char **buff, char **msg)
{
	int i = 0;
	char *newbuff;

	*msg = 0;
	if (*buff == 0)
		return (0);

	while ((*buff)[i])
	{
		if ((*buff)[i] == '\n')
		{
			newbuff = calloc(strlen(*buff + i + 1) + 1, sizeof(*newbuff));
			if (!newbuff)
				return (-1);
			strcpy(newbuff, (*buff + i + 1));
			*msg = *buff;
			(*msg)[i + 1] = 0;
			*buff = newbuff;
			return (1);
		}
		++i;
	}
	return (0);
}

// Функция для отправки сообщения всем клиентам, кроме отправителя.

void send_msg(int fd) {
    for (int sockId = 3; sockId <= maxSock; sockId++) {
        if (FD_ISSET(sockId, &wrt_set) && sockId != fd) {
            send(sockId, buff_sd, strlen(buff_sd), 0);
            if (msg) send(sockId, msg, strlen(msg), 0);
        }
    }
}

int main(int argc, char **argv) {
// Начало главной функции. Проверяется количество аргументов командной строки.
    if (argc != 2) ft_error("Usage: ./server <port>\n");
// Объявляются переменные для сокетов и структур адресов.    
    int sockfd, connfd, cliId = 0;
    struct sockaddr_in servaddr = {0}, cliaddr;
    socklen_t len_cli = sizeof(cliaddr);
// Настраивается структура адреса сервера.
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    servaddr.sin_port = htons(atoi(argv[1]));
// Создается сокет, привязывается к адресу и начинает прослушивание.    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
        bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ||
        listen(sockfd, SOMAXCONN) < 0) ft_error("Fatal error\n");
// Инициализируется набор активных файловых дескрипторов.
    maxSock = sockfd;
    FD_ZERO(&atv_set);
    FD_SET(sockfd, &atv_set);
// Начало основного цикла. Используется select() для мониторинга сокетов.
    while (1) {
// Обработка нового подключения клиента.        
        rd_set = wrt_set = atv_set;
        if (select(maxSock + 1, &rd_set, &wrt_set, NULL, NULL) <= 0) continue;
        if (FD_ISSET(sockfd, &rd_set)) {
            if ((connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len_cli)) < 0) ft_error("Fatal error\n");
            FD_SET(connfd, &atv_set);
            maxSock = (connfd > maxSock) ? connfd : maxSock;
            g_cliId[connfd] = cliId++;
            sprintf(buff_sd, "server: client %d just arrived\n", g_cliId[connfd]);
            send_msg(connfd);
            cliBuff[connfd] = NULL;
            continue;
        }
// Обработка данных от существующих клиентов. Если клиент отключился,
// он удаляется из списка. Если получены данные, они обрабатываются
// и рассылаются другим клиентам.        
        for (int sockId = 3; sockId <= maxSock; sockId++) {
            if (FD_ISSET(sockId, &rd_set) && sockId != sockfd) {
                int rd = recv(sockId, buff_rd, BUFFER_SIZE - 1, 0);
                if (rd <= 0) {
                    FD_CLR(sockId, &atv_set);
                    sprintf(buff_sd, "server: client %d just left\n", g_cliId[sockId]);
                    send_msg(sockId);
                    free(cliBuff[sockId]);
                    close(sockId);
                } else {
                    buff_rd[rd] = '\0';
                    cliBuff[sockId] = str_join(cliBuff[sockId], buff_rd);
                    while (extract_msg(&cliBuff[sockId], &msg)) {
                        sprintf(buff_sd, "client %d: ", g_cliId[sockId]);
                        send_msg(sockId);
                        free(msg);
                    }
                }
            }
        }
    }
    return 0;
}
