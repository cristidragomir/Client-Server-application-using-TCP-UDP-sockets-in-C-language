#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "queue.h"

#define MAX_PORT 65535
#define MIN_PORT 1024
#define MAX_PENDING_CONNECTIONS 512
#define SOCKADDR_IN_SIZE sizeof(struct sockaddr_in)
#define FD_SET_SIZE sizeof(fd_set)
#define TRUE_VAL 1
#define ID_CLIENT_LEN 11
#define INIT_CLIENTS_NUM 32
#define PAYLOAD_LEN 1560
// 1 + 51 + 1 + 1501 + 4 + 2
// pentru transmiterea unui mesaj UDP la un client TCP
#define TOPIC_LEN 51
#define CONTENT_LEN 1501
#define CHAR_SIZE sizeof(char)
#define EXIT_CODE 1
#define POW_OF_10 100

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)
// Macro pentru programarea defensiva
struct TCPClientsDB {
	uint32_t capacity;
	uint32_t cnt;
	struct Client_info *cls;
};
// Baza de date pentru a retine detaliile clientilor
struct Client_info {
	char id[ID_CLIENT_LEN];
	int8_t is_connected;
	int socket;
	uint16_t port;
	uint32_t ip;
	queue prev_msgs;
};
// O intrare in baza de date
struct TCPmsg {
	char type;
	char client_id[ID_CLIENT_LEN];
	char topic_to_sub_unsub[TOPIC_LEN];
	char sf;
	char buffer[PAYLOAD_LEN];
	struct sockaddr_in sender_info;
};
// Structura unui mesaj peste TCP
struct UDPmsg {
	char topic[TOPIC_LEN];
	char type;
	char content[CONTENT_LEN];
};
// Structura unui mesaj peste UDP
struct topics_cls {
	char topic[TOPIC_LEN];
	list subs;
};
// Structura in care vom retine numele unui topic si
// o lista cu abonamentele la aceasta
struct Subscription{
	struct Client_info *client;
	int sf;
};
// Structura unui abonament in care se retine un pointer
// catre detaliile unui client din baza de date TCPClientsDB
// si daca acel abonament trebuie sa beneficieze de 
// store-and-forward sau nu
void disable_neagle_algorithm(int socket);
struct TCPClientsDB *init_clients_db();
int parse_message_tcp(struct TCPmsg *msg_recv, char *buffer);
int parse_message_udp(struct UDPmsg *msg_recv, char *buffer);
void display_type_1_msg(char *buffer);
void display_type_2_msg(struct TCPmsg *rec_msg);
void display_type_3_msg(struct TCPmsg *rec_msg);
void display_udp_msg(struct UDPmsg *recv_msg, 
	struct sockaddr_in *sender_details);
