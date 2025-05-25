#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1025
#define MAX_DATA_LENGTH 256

// случайный байт - 207 / 102 / 103 etc.
uint8_t random_byte() {
    return rand() % 256;
}

// Генерация запроса для чтения всех регистров Modbus
void generate_modbus_read_request(uint8_t *data, int *data_length) {
    // запрос на чтение всех регистров функции 0x03
    data[0] = 0x00;  // Идентификатор транзакции (можно оставить 0)
    data[1] = 0x01;  // Идентификатор протокола (можно оставить 0)
    data[2] = 0x00;  // Номер модуля (можно оставить 0)
    data[3] = 0x00;  // Номер модуля (можно оставить 0)
    data[4] = 0x00;  // Длина оставшейся части сообщения (high byte)
    data[5] = 0x06;  // Длина оставшейся части сообщения (low byte)
    data[6] = 0x01;  // Уникальный адрес сервера Modbus
    data[7] = 0x03;  // Код функции Modbus для чтения регистров
    data[8] = 0x00;  // Начальный адрес (high byte)
    data[9] = 0x00;  // Начальный адрес (low byte)
    data[10] = 0x00; // Количество регистров для чтения (high byte)
    data[11] = 0x64; // Количество регистров для чтения (low byte)

    *data_length = 12; // какой длины читать
}

// Генерация запроса для записи одного регистра Modbus
void generate_modbus_write_request(uint8_t *data, int *data_length, uint8_t register_addr, uint8_t byte) {
    // Модбус запрос на запись одного регистра функции 0x06
    data[0] = 0x00;  // Идентификатор транзакции (можно оставить 0)
    data[1] = 0x01;  // Идентификатор протокола (можно оставить 0)
    data[2] = 0x00;  // Номер модуля (можно оставить 0)
    data[3] = 0x00;  // Номер модуля (можно оставить 0)
    data[4] = 0x00;  // Длина оставшейся части сообщения (high byte)
    data[5] = 0x06;  // Длина оставшейся части сообщения (low byte)
    data[6] = 0x01;  // Уникальный адрес сервера Modbus
    data[7] = 0x06;  // Код функции Modbus для записи регистра
    data[8] = 0x00;  // Номер регистра для записи (high byte)
    data[9] = register_addr; // Номер регистра для записи (low byte)
    data[10] = 0x00; // Значение для записи в регистр (high byte)
    data[11] = byte; // Значение для записи в регистр (low byte)

    *data_length = 12; // Длина сообщения для записи одного регистра Modbus
}

// Функция для отправки запроса к серверу
bool send_modbus_request(const char *ip, int port, uint8_t *data, int data_length) {
    int sockfd;
    struct sockaddr_in server_addr;

    // Создание сокета
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return false;
    }

    // Установка параметров адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);
    memset(&(server_addr.sin_zero), '\0', 8);

    // Подключение к серверу
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        close(sockfd);
        return false;
    }

    // Отправка запроса к серверу Modbus
    if (send(sockfd, data, data_length, 0) == -1) {
        perror("send");
        close(sockfd);
        return false;
    }

    // Закрытие сокета
    close(sockfd);
    return true;
}

// Функция для получения ответа от сервера Modbus и вывода считанных регистров
void read_registers_and_print(const char *ip, int port) {
    uint8_t modbus_data[MAX_DATA_LENGTH];
    int data_length;

    // Генерация запроса для чтения всех регистров Modbus
    generate_modbus_read_request(modbus_data, &data_length);

    // Отправка запроса к серверу Modbus
    if (!send_modbus_request(ip, port, modbus_data, data_length)) {
        printf("Ошибка отправки Modbus запроса на чтение регистров.\n");
        return;
    }

    // Ждем немного перед отправкой запроса на запись
    usleep(100000); // Подождать 100 миллисекунд
}

// Функция для записи рандомных байтов в рандомные регистры
void write_random_bytes_to_registers(const char *ip, int port) {
    // Записываем рандомные байты в рандомные регистры
    for (int i = 0; i < 10; i++) {
        uint8_t modbus_data[MAX_DATA_LENGTH];
        int data_length;

        // Генерация случайного регистра и случайного байта
        uint8_t register_addr = rand() % 100;
        uint8_t random_data = random_byte();

        // Генерация запроса для записи одного регистра Modbus
        generate_modbus_write_request(modbus_data, &data_length, register_addr, random_data);

        // Отправка запроса к серверу Modbus
        if (!send_modbus_request(ip, port, modbus_data, data_length)) {
            printf("Ошибка отправки Modbus запроса на запись регистра.\n");
            return;
        }
    }

    // Ждем немного перед отправкой запроса на чтение
    usleep(100000); // Подождать 100 миллисекунд
}

int main() {
    srand(time(NULL));

    // Считывание и вывод всех регистров
    printf("Считанные регистры:\n");
    read_registers_and_print(SERVER_IP, SERVER_PORT);

    // Запись рандомных байтов в рандомные регистры
    write_random_bytes_to_registers(SERVER_IP, SERVER_PORT);

    // Считывание и вывод всех регистров после записи
    printf("\nСчитанные регистры после записи:\n");
    read_registers_and_print(SERVER_IP, SERVER_PORT);

    return 0;
}
