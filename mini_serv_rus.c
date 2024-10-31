#include <stdlib.h>      // Для использования функций памяти и конвертации типа
#include <string.h>      // Для работы со строками (strcpy, strcat и т.д.)
#include <unistd.h>      // Для системных вызовов, таких как close, write и select
#include <stdio.h>       // Для работы с вводом/выводом (например, printf, sprintf)
#include <sys/select.h>  // Для использования функции select для мониторинга файловых дескрипторов
#include <arpa/inet.h>   // Для работы с сетевыми функциями и структурами, такими как sockaddr_in

#define MAX_CLIENTS 5000     // Максимальное количество клиентов
#define BUFFER_SIZE 1001     // Размер буфера для чтения и записи данных

int maxSock;                 // Переменная для хранения максимального файлового дескриптора
char *msg = NULL;            // Указатель на сообщение, которое будет отправлено клиентам
int g_cliId[MAX_CLIENTS];     // Массив для хранения уникальных ID клиентов, привязанных к файловым дескрипторам
char *cliBuff[MAX_CLIENTS];   // Массив указателей для хранения буферов клиентов (неконечные сообщения)
char buff_sd[BUFFER_SIZE];    // Буфер для отправки данных клиентам
char buff_rd[BUFFER_SIZE];    // Буфер для чтения данных от клиентов
fd_set rd_set, wrt_set, atv_set; // Наборы дескрипторов для чтения, записи и активных сокетов

// Функция для обработки фатальных ошибок. Выводит сообщение об ошибке и завершает программу.
void ft_error(const char *s) {
    write(2, s, strlen(s));   // Выводит сообщение об ошибке в стандартный поток ошибок
	exit(1);     // Завершает выполнение программы с кодом ошибки
}

// Функция для извлечения сообщения из буфера клиента.
int extract_message(char **buf, char **msg) {
	char *newbuf;
	int i;

	*msg = 0; // Инициализируем указатель сообщения как NULL
	if (*buf == 0) // Если буфер пуст, возвращаем 0
		return (0);
	i = 0;
	// Проходим по буферу, пока не найдем символ новой строки '\n'
	while ((*buf)[i]) {
		if ((*buf)[i] == '\n') {
			// Создаем новый буфер, который будет содержать данные после '\n'
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0) // Проверка на успешное выделение памяти
				return (-1);
			// Копируем остаток буфера в новый буфер
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;    // Сообщение — это оригинальный буфер до '\n'
			(*msg)[i + 1] = 0; // Завершаем сообщение символом '\0'
			*buf = newbuf;  // Обновляем указатель буфера на оставшуюся часть
			return (1);     // Возвращаем 1, что означает, что сообщение извлечено
		}
		i++;
	}
	return (0);  // Возвращаем 0, если сообщение не завершено
}

// Функция для объединения двух строк (буферов)
char *str_join(char *buf, char *add) {
	char *newbuf;
	int len;

	// Определяем длину существующего буфера
	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	// Выделяем память для нового буфера, который будет содержать оба буфера
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0) // Проверка успешности выделения памяти
		return (0);
	newbuf[0] = 0; // Инициализируем новый буфер как пустую строку
	if (buf != 0)
		strcat(newbuf, buf); // Если есть существующий буфер, добавляем его
	free(buf);              // Освобождаем старый буфер
	strcat(newbuf, add);    // Добавляем новый текст в конец
	return (newbuf);        // Возвращаем новый объединенный буфер
}

// Функция для отправки сообщения всем активным клиентам
void send_msg() {
	for (int sockId = 3; sockId <= maxSock; sockId++) {  // Проходим по всем сокетам
		if (FD_ISSET(sockId, &wrt_set)) {               // Проверяем, доступен ли сокет для записи
			send(sockId, buff_sd, strlen(buff_sd), 0);  // Отправляем основной буфер сообщения
			if (msg) send(sockId, msg, strlen(msg), 0); // Отправляем дополнительное сообщение, если оно есть
		}
	}
}

int main(int argc, char **argv) {
	if (argc != 2) // Проверяем, что передан правильный аргумент (порт)
		ft_error("Wrong number of arguments\n");

	int sockfd, connfd, cliId = 0; // Переменные для серверного сокета, клиентского сокета и ID клиента
	struct sockaddr_in servaddr = {0}, cliaddr; // Структуры для адресов сервера и клиента
	socklen_t len_cli = sizeof(cliaddr); // Размер структуры адреса клиента

	// Настраиваем серверный адрес (127.0.0.1 и порт из аргумента)
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	servaddr.sin_port = htons(atoi(argv[1]));

	// Создаем серверный сокет, привязываем его и начинаем слушать подключения
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
		bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ||
		listen(sockfd, SOMAXCONN) < 0)
			ft_error("Fatal error\n");

	FD_SET(maxSock = sockfd, &atv_set); // Добавляем серверный сокет в набор активных дескрипторов

	while (1) {
		rd_set = wrt_set = atv_set; // Копируем набор активных сокетов для select
		if (select(maxSock + 1, &rd_set, &wrt_set, NULL, NULL) <= 0)
			continue; // Ждем активности на одном из сокетов
		// Обрабатываем новые подключения
		if (FD_ISSET(sockfd, &rd_set)) {
			if ((connfd = accept(sockfd, (struct sockaddr *) &cliaddr, &len_cli)) < 0)
				ft_error("Fatal error\n");
			FD_SET(connfd, &atv_set); // Добавляем новый сокет в набор активных
			maxSock = (connfd > maxSock) ? connfd : maxSock; // Обновляем максимальный сокет
			g_cliId[connfd] = cliId++; // Присваиваем клиенту уникальный ID
			sprintf(buff_sd, "server: client %d just arrived\n", g_cliId[connfd]); // Формируем сообщение о новом клиенте
			send_msg(); // Отправляем сообщение всем клиентам
			cliBuff[connfd] = NULL; // Инициализируем буфер для нового клиента
		}
		// Обрабатываем данные от подключенных клиентов
		for (int sockId = 3; sockId <= maxSock; sockId++) {
			if (FD_ISSET(sockId, &rd_set) && sockId != sockfd) { // Если есть данные от клиента
				int rd = recv(sockId, buff_rd, BUFFER_SIZE - 1, 0); // Читаем данные
				if (rd <= 0) { // Если клиент отключился
					FD_CLR(sockId, &atv_set); // Убираем сокет из активных
					sprintf(buff_sd, "server: client %d just left\n", g_cliId[sockId]); // Сообщение об отключении клиента
					send_msg(); // Отправляем сообщение всем клиентам
					free(cliBuff[sockId]); // Освобождаем буфер клиента
					close(sockId); // Закрываем сокет
				} else { // Если данные успешно прочитаны
					buff_rd[rd] = '\0'; // Завершаем строку нулевым символом
					cliBuff[sockId] = str_join(cliBuff[sockId], buff_rd); // Добавляем полученные данные в буфер клиента
					while(extract_message(&cliBuff[sockId], &msg)) { // Извлекаем сообщения
						sprintf(buff_sd, "client %d: ", g_cliId[sockId]); // Формируем сообщение с ID клиента
						send_msg(); // Отправляем сообщение всем клиентам
						free(msg); // Освобождаем память под сообщение
					}
				}
			}
		}
	}
	return 0;
}
