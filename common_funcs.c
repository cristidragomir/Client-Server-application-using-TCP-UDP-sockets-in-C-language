#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>


#include "tools.h"

void disable_neagle_algorithm(int socket) {
	uint32_t on = 1;
	int disabling_status = setsockopt(socket,
							IPPROTO_TCP,
							TCP_NODELAY,
							(char *) &on,
							sizeof(uint32_t));
	DIE(disabling_status < 0, 
		"Algoritmul lui Neagle nu a putut fi dezactivat");
}

struct TCPClientsDB *init_clients_db() {
	struct TCPClientsDB *cl_db;
	cl_db = malloc(sizeof(struct TCPClientsDB));
	cl_db->capacity = INIT_CLIENTS_NUM;
	cl_db->cnt = 0;
	cl_db->cls = malloc(INIT_CLIENTS_NUM * sizeof(struct Client_info));
	return cl_db;
}

void display_type_1_msg(char *buffer) {
	printf("TIP MSJ: %c\n", buffer[0]);
	printf("ID_CLIENT: %s\n", buffer + sizeof(char));
	uint16_t test_port;
	memcpy(&test_port, buffer + sizeof(char) + ID_CLIENT_LEN + 1, sizeof(uint16_t));
	test_port = ntohs(test_port);
	printf("PORT: %hu\n", test_port);
	struct in_addr test_ip;
	memcpy(&test_ip.s_addr, (buffer + sizeof(char) + ID_CLIENT_LEN + 1 + sizeof(unsigned short)), sizeof(uint32_t));
	printf("IPv4: %s\n", inet_ntoa(test_ip));
} 
