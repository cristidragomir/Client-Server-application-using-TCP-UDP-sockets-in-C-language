#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "tools.h"

void send_client_id(int to_send_sock, char *client_id, struct sockaddr_in *client_details) {
	char *buffer = calloc(PAYLOAD_LEN, sizeof(char));
	char msg_type = '1';
	memcpy(buffer, &msg_type, sizeof(char));
	memcpy(buffer + sizeof(char), client_id, ID_CLIENT_LEN);
	memcpy(buffer + sizeof(char) + ID_CLIENT_LEN + 1, &client_details->sin_port, 
		sizeof(unsigned short));
	memcpy(buffer + sizeof(char) + ID_CLIENT_LEN + 1 + sizeof(unsigned short),
		&client_details->sin_addr.s_addr,
		sizeof(unsigned long));
	display_type_1_msg(buffer);
	send(to_send_sock, buffer, PAYLOAD_LEN, 0);
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	DIE (argc < 4, "Argumente insuficiente pentru client");
	DIE ((strlen(argv[1]) > 10), "ID client prea lung");
	int tcp_sock_descr = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sock_descr < 0, "Eroare initializare socket");
	// Am initializat socket-ul clientului TCP
	int port = atoi(argv[3]);
	DIE((port > MAX_PORT || port < MIN_PORT), "Port invalid");
	// Am prelucrat portul dat ca parametru
	struct sockaddr_in *client_details;
	int chk_ret;
	client_details = calloc(1, SOCKADDR_IN_SIZE);
	DIE(client_details == NULL, "Eroare alocare memorie");
	client_details->sin_family = AF_INET;
	client_details->sin_port = htons((uint16_t)port);
	chk_ret = inet_aton(argv[2], &client_details->sin_addr);
	DIE(chk_ret == 0, "Adresa IP data ca parametru este invalida");
	// printf("%s\n", inet_ntoa(client_details->sin_addr)); - ASTA MERGE
	// Am initializat detaliile de conexiune ale clientului
	fd_set *inputs = malloc(FD_SET_SIZE);
	DIE(inputs == NULL, "Eroare alocare memorie");
	fd_set *prev_inputs = malloc(FD_SET_SIZE);
	DIE(prev_inputs == NULL, "Eroare alocare memorie");
	int max_input_rank;
	FD_ZERO(inputs);
	FD_ZERO(prev_inputs);
	FD_SET(tcp_sock_descr, inputs);
	max_input_rank = tcp_sock_descr;
	FD_SET(0, inputs);
	// Am initializat sursele din care clientul poate citi
	chk_ret = connect(tcp_sock_descr, 
		(struct sockaddr *)client_details, SOCKADDR_IN_SIZE);
	DIE(chk_ret < 0, "Eroare la conectarea cu serverul");
	send_client_id(tcp_sock_descr, argv[1], client_details);
	while (TRUE_VAL) {
		
	}
	return 0;
}