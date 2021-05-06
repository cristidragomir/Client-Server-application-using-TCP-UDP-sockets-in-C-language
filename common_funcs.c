#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>


#include "tools.h"

void disable_neagle_algorithm(int socket) {
	int on = 1;
	int disabling_status = setsockopt(socket,
							IPPROTO_TCP,
							TCP_NODELAY,
							(char *) &on,
							sizeof(int));
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

int parse_message(struct TCPmsg *msg_recv, char *buffer) {
	// msg_recv e deja initializat
	msg_recv->type = buffer[0];
	if (msg_recv->type == '1') {
		uint32_t actual_id_len = strlen(buffer + sizeof(char));
		memcpy(msg_recv->client_id, buffer + sizeof(char), actual_id_len);
		msg_recv->client_id[actual_id_len] = '\0';
		return 1;
		// S-a procesat un mesaj TCP in care clientul isi trimite propriul ID
	}
	return -1;
}

void display_type_1_msg(char *buffer) {
	struct TCPmsg recv_msg;
	DIE(parse_message(&recv_msg, buffer) < 0, "Eroare la parsarea buffer-ului");
	printf("TIP MSJ: %c\n", recv_msg.type);
	printf("ID_CLIENT: %s\n", recv_msg.client_id);
}
