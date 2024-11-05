#7
#include <string.h> // Для работы со строками
#include <unistd.h> // Для системных вызовов UNIX
#include <stdlib.h> // Для общих функций стандартной библиотеки
#include <stdio.h> // Для стандартного ввода-вывода
#include <sys/socket.h> // Для работы с сокетами
#include <sys/select.h> // Для функции select()
#include <netinet/in.h> // Для структур интернет-адресов
#7
int sockfd, maxfd; // Главный сокет сервера и максимальный дескриптор
struct sockaddr_in servaddr; // Структура для адреса сервера

fd_set currfd, readfd; // Наборы дескрипторов для текущих и читаемых сокетов

int ids[FD_SETSIZE]; // Массив для хранения ID клиентов
int cnt = 0; // Счетчик клиентов

char serv_buf[50]; // Буфер для сообщений сервера
char cli_buf[50]; // Буфер для сообщений клиента

void err42(char *s) {
    write(2, s, strlen(s));
    exit(1);
}
#4
void broadcast(int clientfd, char *buffer, int len) {
    for (int fd = 0; fd <= maxfd; fd++)
        if (fd != clientfd) // Не отправлять сообщение отправителю
            send(fd, buffer, len, 0);
}
#12
void send_msg(int fd, int len) {
    sprintf(serv_buf, "client %d: ", ids[fd]); // Формировать префикс с ID клиента
    broadcast(fd, serv_buf, strlen(serv_buf)); // Отправить префикс

    char send[2] = {}; // Буфер для посимвольной отправки
    for (int i = 0; i < len; i++) {
        send[0] = cli_buf[i];
        broadcast(fd, send, strlen(send));
        // Если следующий символ перевод строки - добавить префикс
        if (i + 1 < len && send[1] == '\n')
            broadcast(fd, serv_buf, strlen(serv_buf));
        // Если достигнут конец буфера - получить следующую порцию данных
        if (i + 1 == len && len == 50) {
            len = recv(fd, cli_buf, 50, 0);
            i = -1;
        }
    }
}

int main(int ac, char **av) {
    #9
    if (ac != 2)
        err42("Wrong number of arguments\n");
    // Создаем TCP сокет (AF_INET - IPv4, SOCK_STREAM - потоковый сокет)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        err42("Fatal error\n");
    // Очищаем структуру адреса сервера
    bzero(&servaddr, sizeof(servaddr));
    // Настраиваем параметры адреса сервера:
    servaddr.sin_family = AF_INET;  // Семейство протоколов IPv4
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); // Преобразуем номер порта из аргумента в сетевой порядок байтов
    // Привязка сокета к адресу
    // Приведение типа к struct sockaddr необходимо для совместимости
    #4
    if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
         err42("Fatal error\n");
    // Перевод сокета в режим прослушивания
    // Максимальная длина очереди входящих подключений - 10
    if (listen(sockfd, 10) != 0)
        err42("Fatal error\n");
    // Инициализация наборов дескрипторов
    #4
    FD_ZERO(&currfd);  // Очищаем набор текущих дескрипторов
    FD_ZERO(&readfd);     // Очищаем набор дескрипторов для чтения
    FD_SET(sockfd, &currfd);  // Добавляем серверный сокет в набор текущих дескрипторов
    maxfd = sockfd;  // Начальное значение максимального дескриптора - серверный сокет
    // Основной цикл обработки событий
    while (1) {
        // Копируем набор текущих дескрипторов, так как select() его модифицирует
        #3
        readfd = currfd;
        // Ждем события на любом из сокетов
        // maxfd + 1 - количество проверяемых дескрипторов
        // Второй и третий параметры (0) - не отслеживаем события записи и ошибок
        // Последний параметр (0) - бесконечное ожидание
        if (select(maxfd + 1, &readfd, 0, 0, 0) < 0)
            continue;
        // Проверяем все дескрипторы от 0 до maxfd
        #3
        for (int fd = 0; fd <= maxfd; fd++) {
            // Пропускаем дескрипторы, на которых не было событий
            if (!(FD_ISSET(fd, &readfd)))
                continue;
            // Если событие на серверном сокете - это новое подключение
            #8
            if (sockfd == fd) { // Если это серверный сокет - принять новое подключение
                int clientfd = accept(sockfd, 0, 0);
                if (clientfd > maxfd)
                    maxfd = clientfd;
                ids[clientfd] = cnt++;
                // Добавляем клиентский сокет в набор отслеживаемых
                FD_SET(clientfd, &currfd);
                // Формируем и отправляем сообщение о подключении нового клиента
                sprintf(serv_buf, "server: client %d just arrived\n", ids[clientfd]);
                broadcast(clientfd, serv_buf, strlen(serv_buf));
            } else { // Если это клиентский сокет - обработать данные
                // Читаем данные от клиента
                #8
                int len = recv(fd, cli_buf, 50, 0);
                // Если получены данные
                if (len > 0) {
                    // Отправляем сообщение всем остальным клиентам
                    send_msg(fd, len);
                } else { // Если клиент отключился
                    // Формируем и отправляем сообщение об отключении клиента
                    sprintf(serv_buf, "server: client %d just left\n", ids[fd]);
                    broadcast(fd, serv_buf, strlen(serv_buf));
                    // Удаляем сокет клиента из набора отслеживаемых
                    FD_CLR(fd, &currfd);
                    // Закрываем сокет
                    close(fd);
                }
            }
        }
    }
}