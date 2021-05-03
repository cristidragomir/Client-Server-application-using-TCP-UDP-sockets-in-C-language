#include <stdio.h>
#include <stdlib.h>

#define MAX_PORT 65535
#define MIN_PORT 1024
#define MAX_PENDING_CONNECTIONS 512
#define SOCKADDR_IN_SIZE sizeof(struct sockaddr_in)
#define FD_SET_SIZE sizeof(fd_set)
#define TRUE_VAL 1
#define ID_CLIENT_LEN 11
#define INIT_CLIENTS_NUM 32
#define PAYLOAD_LEN 1500
#define EXIT_CODE 1

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

struct TCPClientsDB {
	uint32_t capacity;
	uint32_t cnt;
	struct Client_info *cls;
};

struct Client_info {
	char id[ID_CLIENT_LEN];
	int8_t is_connected;
	int socket;
	uint16_t port;
	uint32_t ip;
};

void disable_neagle_algorithm(int socket);
struct TCPClientsDB *init_clients_db();
void display_type_1_msg(char *buffer);