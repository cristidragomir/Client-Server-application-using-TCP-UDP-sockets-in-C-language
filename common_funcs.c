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

int parse_message_tcp(struct TCPmsg *msg_recv, char *buffer) {
	// msg_recv e deja initializat
	msg_recv->type = buffer[0];
	if (msg_recv->type == '1') {
		uint32_t actual_id_len = strlen(buffer + CHAR_SIZE);
		memcpy(msg_recv->client_id, buffer + CHAR_SIZE, actual_id_len);
		msg_recv->client_id[actual_id_len] = '\0';
		return 1;
		// S-a procesat un mesaj TCP in care clientul isi trimite propriul ID
	} else if (msg_recv->type == 'E') {
		return 1;
	}
	return -1;
}

int parse_message_udp(struct UDPmsg *msg_recv, char *buffer) {
	uint32_t actual_topic_len = strlen(buffer);
	memcpy(msg_recv->topic, buffer, (actual_topic_len + 1));
	// Se copiaza topicul si se adauga '\0' la final
	msg_recv->type = *(buffer + TOPIC_LEN - 1);
	// S-a preluat tipul de date
	char *content_ptr = buffer + TOPIC_LEN + CHAR_SIZE - 1;
	switch ((uint32_t)msg_recv->type)
	{
		case 0: ;
			// nu merge sa fie pusa o declaratie de variabila imediat dupa un label
			memcpy(msg_recv->content, content_ptr, 5 * CHAR_SIZE);
			// S-a preluat 1 byte de semn si 4 bytes ce encodeaza un numar intreg
			break;
		case 1: ;
			memcpy(msg_recv->content, content_ptr, 2 * CHAR_SIZE);
			// S-au preluat 2 bytes ce reprezinta un uint16_t	
			break;
		case 2: ;
			memcpy(msg_recv->content, content_ptr, 6 * CHAR_SIZE);
			// S-a preluat 1 byte de semn, 4 bytes - uint32_t, 1 byte - uint8_t
			break;
		case 3:	;
			uint32_t actual_content_len = strlen(content_ptr);
			memcpy(msg_recv->content, content_ptr, (actual_content_len + 1));
			// S-a preluat contentul si se adauga '\0' la final
			break;
	}
	return 1;
}

void display_type_1_msg(char *buffer) {
	struct TCPmsg recv_msg;
	DIE(parse_message_tcp(&recv_msg, buffer) < 0, "Eroare la parsarea buffer-ului");
	printf("TIP MSJ: %c\n", recv_msg.type);
	printf("ID_CLIENT: %s\n", recv_msg.client_id);
}

void display_udp_msg(struct UDPmsg *recv_msg) {
	printf("UDP TOPIC: %s\n", recv_msg->topic);
	printf("UDP TYPE: %d\n", (int)recv_msg->type);
	printf("UDP ");
	switch ((uint32_t)recv_msg->type) {
		case 0: ;
		// nu merge sa fie pusa o declaratie de variabila imediat dupa un label
			uint32_t number;
			memcpy(&number, recv_msg->content + CHAR_SIZE, sizeof(uint32_t));
			number = ntohl(number);
			printf("INT: ");
			if (*recv_msg->content == 0) {
				printf("%u\n", number);
			} else {
				printf("-%u\n", number);
			}
			break;
		case 1: ;
			uint16_t short_real;
			memcpy(&short_real, recv_msg->content + CHAR_SIZE, sizeof(uint16_t));
			printf("SHORT REAL: %hu", short_real);
			//printf("SHORT_REAL: %hu.%hu", (uint16_t)(short_real / POW_OF_10), (uint16_t)(short_real % POW_OF_10));
			break;
		case 2: ;

			break;
		case 3: ;
			printf("UDP STRING: %s\n", recv_msg->content);
			break;
	}
	printf("\n");
}
