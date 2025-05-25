#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <libwebsockets.h>
#include <pthread.h>
#include <unistd.h>
#include <jansson.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define STR_LEN 512 // TODO: изменить на определение размера во время исполнения
#define CHUNK_SIZE 4096
#define TRUE 1
#define FALSE 0
#define MAX_VARS %(num_links)s
#define SERVER_IP "%(ip)s"
#define SERVER_PORT %(port)s
#define MAX_DATA_LENGTH 256
#define BIGSIZE 1024
#define MAX_HEADER_SIZE 14
#define by_change %(WriteChangeFlag)s

typedef enum {
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_REAL,
    TYPE_INT,
    TYPE_VOID,
} VariableType;

const int VariableTypeSizes[] = {
	4, // TYPE_FLOAT
	2, // TYPE_BOOL
	4, // TYPE_REAL
	4, // TYPE_INT
	0  // TYPE_VOID
};

typedef union {
        int32_t i_value;
        float f_value;
        struct {
            uint8_t byte1;
            uint8_t byte2;
        } b_value;
} Value;

typedef struct {
    int start_register; // Начальное значение регистра
    VariableType type; // Тип переменной
    Value current;      // Текущее значение
    Value last;         // Предыдущее значение
} Variable;


//Массив переменных
Variable variables[MAX_VARS];

//Pointers:
%(carray_p)s
void *void_ptr;

Variable *pointers_array[] = %(pointers_arr_str)s

// Beremiz api
//int __init_%%(locstr)s(int argc, char **argv);
//void __cleanup_%%(locstr)s(void);
//void __retrieve_%%(locstr)s(void);
//void __publish_%%(locstr)s(void);

// Генерация запроса для чтения всех регистров Modbus
void generate_modbus_read_request(uint8_t *data, int *data_length) {
    // Модбус запрос на чтение всех регистров функции 0x03
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
    *data_length = 12; // Длина сообщения для чтения регистров Modbus
}

// Генерация запроса для записи одного регистра Modbus
void generate_modbus_write_request(uint8_t *data, int *data_length, uint8_t register_addr, uint8_t *bytes) {
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
    data[10] = bytes[0]; // Значение для записи в регистр (high byte)
    data[11] = bytes[1]; // Значение для записи в регистр (low byte)
    *data_length = 12; // Длина сообщения для записи одного регистра Modbus
}

// Генерация запроса для записи нескольких регистров Modbus
void generate_modbus_write_multiple_request(uint8_t *data, int *data_length, uint8_t start_register, uint8_t register_count, uint8_t *values) {
    // Модбус запрос на запись нескольких регистров функции 0x10
    data[0] = 0x00;                 // Идентификатор транзакции (можно оставить 0)
    data[1] = 0x01;                 // Идентификатор протокола (можно оставить 0)
    data[2] = 0x00;                 // Номер модуля (можно оставить 0)
    data[3] = 0x00;                 // Номер модуля (можно оставить 0)
    data[4] = 0x00;                 // Длина оставшейся части сообщения (high byte)
    data[5] = register_count * 2 + 7; // Длина оставшейся части сообщения (low byte)
    data[6] = 0x01;                 // Уникальный адрес сервера Modbus
    data[7] = 0x10;                 // Код функции Modbus для записи нескольких регистров
    data[8] = 0x00;                 // Начальный адрес (high byte)
    data[9] = start_register;       // Начальный адрес (low byte)
    data[10] = 0x00;                // Количество регистров для записи (high byte)
    data[11] = register_count;      // Количество регистров для записи (low byte)
    data[12] = register_count * 2;  // Количество байт данных
    for (int i = 0; i < register_count * 2; i+=2) {
        data[13 + i] = values[i+1];
        data[14 + i] = values[i];  // Значения для записи в регистры
    }
    *data_length = 13 + register_count * 2; // Длина сообщения для записи нескольких регистров Modbus
}

void print_modbus_request(uint8_t *data, int data_length) {
    printf("Sending Modbus request:\n");
    for (int i = 0; i < data_length; i++) {
        printf("%%02X ", data[i]); // Выводим каждый байт запроса в шестнадцатеричном формате
    }
    printf("\n"); // Переход на новую строку для читаемости
}

void print_modbus_write_request(uint8_t *data, int data_length) {
    printf("Sending Modbus write request:\n");
    for (int i = 0; i < data_length; i++) {
        printf("%%02X ", data[i]); // Выводим каждый байт запроса в шестнадцатеричном формате
    }
    printf("\n"); // Переход на новую строку для читаемости
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

    print_modbus_request(data, data_length);

    // Отправка запроса на чтение всех регистров
    if (send(sockfd, data, data_length, 0) == -1) {
        perror("send");
        close(sockfd);
        return false;
    }

    // Получение ответа от сервера
    uint8_t response[MAX_DATA_LENGTH];
    int bytes_received = recv(sockfd, response, MAX_DATA_LENGTH, 0);
    if (bytes_received == -1) {
        perror("recv");
        close(sockfd);
        return false;
    }

    // Вывод считанных регистров
    printf("Memory status:\n");
    for (int i = 0; i < bytes_received; i++) {
        printf("%%d ", response[i]);
    }
    printf("\n");

    // Закрытие сокета
    close(sockfd);
    return true;
}

bool send_modbus_write_request(const char *ip, int port, uint8_t *data, int data_length) {
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

    print_modbus_write_request(data, data_length);

    // Отправка запроса на запись регистров
    if (send(sockfd, data, data_length, 0) == -1) {
        perror("send");
        close(sockfd);
        return false;
    }

    // Закрытие сокета
    close(sockfd);
    return true;
}

// Функция для записи одного значения переменной в регистр Modbus
void write_single_value_to_register(const char *ip, int port, uint8_t register_addr, uint8_t *data) {
    uint8_t modbus_data[MAX_DATA_LENGTH];
    int data_length;

    // Генерируем запрос для записи одного регистра Modbus
    generate_modbus_write_request(modbus_data, &data_length, register_addr, data);
    printf("SINGLE\n");
    // Отправляем запрос к серверу Modbus
    if (!send_modbus_write_request(ip, port, modbus_data, data_length)) {
        printf("Ошибка отправки Modbus запроса на запись регистра.\n");
        return;
    }
}

// Функция для записи нескольких значений переменных в регистры Modbus
void write_multiple_values_to_registers(const char *ip, int port, uint16_t start_register, uint16_t register_count, uint16_t *values) {
    uint8_t modbus_data[MAX_DATA_LENGTH];
    int data_length;

    // Генерируем запрос для записи нескольких регистров Modbus
    generate_modbus_write_multiple_request(modbus_data, &data_length, start_register, register_count, (uint8_t *)values);
    printf("MULTI");
    // Отправляем запрос к серверу Modbus
    if (!send_modbus_write_request(ip, port, modbus_data, data_length)) {
        printf("Ошибка отправки Modbus запроса на запись регистров.\n");
        return;
    }
}

void __retrieve_%(locstr)s(void)
{
//    return;
    printf("\n\n\nRETRIEVE BLOCK ...\n");
    uint8_t modbus_data[MAX_DATA_LENGTH];
    int data_length;

    // Генерация запроса для чтения всех регистров Modbus
    generate_modbus_read_request(modbus_data, &data_length);

    // Отправка запроса к серверу Modbus и вывод считанных регистров
    if (send_modbus_request(SERVER_IP, SERVER_PORT, modbus_data, data_length)) {
        printf("Request sent successfully\n\n\n");
    } else {
        printf("Error sending request\n");
    }

    get_variables(SERVER_IP, SERVER_PORT);

    int underline_count = 188;
    // Вывод подчеркиваний
    for (int i = 0; i < underline_count; i++) {
        printf("_");
    }
}

void __cleanup_%(locstr)s(void)
{
//    return;
      printf("111\n");

//    printf(stderr, "mbus runtime cleanup\n");
}

void __publish_%(locstr)s(void)
{
//    return;
    printf("\n\n\nPUBLISH BLOCK ...\n");
    make_buf(SERVER_IP, SERVER_PORT);
    printf("PUBLISHED DATA");
}

int __init_%(locstr)s (int argc, char **argv)
{
//    return 0;
%(carray)s
    printf("STARTING MBUS ...\n");
    return 0;
}

void fill_buffer(int8_t *pos, int size, void *from) {
	for (int i = 0; i < size; ++i) {
		pos[i] = ((int8_t*)from)[size - i - 1];
	}
}

void make_buf(const char *ip, int port) {
	int8_t buff[BIGSIZE + MAX_HEADER_SIZE];

	for (int i = 0; variables[i].type != TYPE_VOID; )
	{
		int first_reg_idx, cur_var_idx;
		int register_count_to_write = 0;
		first_reg_idx = cur_var_idx  = variables[i].start_register;
		int8_t *cur_pos = buff;

		while (variables[i].start_register == cur_var_idx && variables[i].type != TYPE_VOID) {
		    if (by_change && variables[i].current.i_value == variables[i].last.i_value) {
            i++;
            continue;
            }
            variables[i].last.i_value = variables[i].current.i_value;
			int last_size = VariableTypeSizes[variables[i].type]; // 4
			fill_buffer(cur_pos, last_size, &variables[i].current); // buff+2, 4, *5
			cur_var_idx += last_size / 2; // 7
			register_count_to_write += last_size / 2; // 4
			cur_pos += last_size; // + buff+2+2
			i++; // 2
		}

		if (register_count_to_write == 1) {
		    write_single_value_to_register(ip, port, first_reg_idx, buff);
		} else {
			write_multiple_values_to_registers(ip, port, first_reg_idx, register_count_to_write, buff);
		}
	}
}

void get_variables(const char *ip, int port) {
    uint8_t modbus_data[MAX_DATA_LENGTH];
    int data_length = 256;

    generate_modbus_read_request(modbus_data, &data_length);

    if (!send_modbus_request(ip, port, modbus_data, data_length)) {
        printf("Ошибка отправки Modbus запроса\n");
        return;
    }

    int response_size = data_length - 9; // Количество байт в ответе, исключая заголовок
    uint8_t *response = modbus_data + 9; // Смещение указателя на начало данных

    for (int i = 0; i < MAX_VARS; i++) {
        if (variables[i].type == TYPE_VOID) continue;

        int start_register = variables[i].start_register;
        int size = VariableTypeSizes[variables[i].type];
        int byte_offset = start_register * 2;

        if (byte_offset + size <= response_size) {
            memcpy(&variables[i].current, response + byte_offset, size);
            printf("DONE");
        } else {
            printf("Ошибка: размер данных превышает размер ответа.\n");
            return;
        }
    }
}




