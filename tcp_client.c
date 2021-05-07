#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "tools.h"

void send_client_id(int to_send_sock, char *client_id) {
	int chk_func;
	char *buffer = calloc(PAYLOAD_LEN, CHAR_SIZE);
	char msg_type = '1';
	memcpy(buffer, &msg_type, CHAR_SIZE);
	memcpy(buffer + CHAR_SIZE, client_id, ID_CLIENT_LEN);
	display_type_1_msg(buffer);
	chk_func = send(to_send_sock, buffer, PAYLOAD_LEN, 0);
	DIE(chk_func < 0, "Eroare trimitere informatii");
}

int read_stdin() {
	char *buffer = calloc(PAYLOAD_LEN, CHAR_SIZE);
	fgets(buffer, PAYLOAD_LEN - 1, stdin);
	if (strncmp(buffer, "exit", 4) == 0) {
		return EXIT_CODE;
	}
	free(buffer);
	return 0;
}

int read_server(int socket) {
	char *buffer = calloc(PAYLOAD_LEN, CHAR_SIZE);
	int chk_func;
	chk_func = recv(socket, buffer, PAYLOAD_LEN, 0);
	DIE(chk_func < 0, "Eroare receptie informatii");
	struct TCPmsg rec_msg;
	DIE(parse_message_tcp(&rec_msg, buffer) < 0, "Eroare parsare buffer");
	if (rec_msg.type == 'E') {
		return EXIT_CODE;
	}
	free(buffer);
	return 0;
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	DIE (argc < 4, "Argumente insuficiente pentru client");
	DIE ((strlen(argv[1]) > 10), "ID client prea lung");
	int tcp_sock_descr = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sock_descr < 0, "Eroare initializare socket");
	// Am initializat socket-ul clientului TCP
	disable_neagle_algorithm(tcp_sock_descr);
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
	send_client_id(tcp_sock_descr, argv[1]);
	while (TRUE_VAL) {
		memcpy(prev_inputs, inputs, FD_SET_SIZE);

		chk_ret = select(max_input_rank + 1, prev_inputs, NULL, NULL, NULL);
		DIE(chk_ret < 0, "Eroare la selectia sursei de input");
		if (FD_ISSET(0, prev_inputs)) {
		// Se citeste de la tastatura
			if (read_stdin() == EXIT_CODE) {
				// Clientul se va inchide
				break;
			}
		} else {
			if (read_server(tcp_sock_descr) == EXIT_CODE) {
				// Serverul se va inchide si, deci, se inchide si clientul
				break;
			}
		}
	}
	FD_CLR(tcp_sock_descr, inputs);
	FD_CLR(0, inputs);
	close(tcp_sock_descr);
	// Am inchis socket-ul clientului
	return 0;
}