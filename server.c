#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "tools.h"

void control_inp_srcs(fd_set *inputs, 
	fd_set *prev_inputs, int *max_input_rank, 
	int inp_index, int server_socket) {

	int new_sock;
	struct sockaddr_in *incoming_client = malloc(SOCKADDR_IN_SIZE);
	DIE(incoming_client == NULL, "Eroare alocare memorie");
	if (inp_index == server_socket) {
		// In acest caz incercam sa primim noi conexiuni catre server
		socklen_t inc_client_size = SOCKADDR_IN_SIZE;
		new_sock = accept(server_socket, (struct sockaddr *)incoming_client, &inc_client_size);
		DIE(new_sock < 0, "Eroare acceptare conexiune noua");
		// Am acceptat o noua conexiune la acest server
		FD_SET(new_sock, inputs);
		if (new_sock > *max_input_rank) {
			*max_input_rank = new_sock;
		}
		// Am introdus in multimea de surse de citire o noua sursa

	}
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	DIE (argc < 2, "Argumente insuficiente pentru server");

	int tcp_sock_descr;
	tcp_sock_descr = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sock_descr < 0, "Nu s-a putut initializa socketul");
	// Am creat socket-ul serverului
	disable_neagle_algorithm(tcp_sock_descr);
	// Am dezactivat algoritmul lui Neagle
	int port = atoi(argv[1]);
	DIE((port > MAX_PORT || port < MIN_PORT), "Port invalid");
	// Am parsat ca numar portul dat ca parametru programului
	struct sockaddr_in *server_details;
	server_details = calloc(1, SOCKADDR_IN_SIZE);
	DIE(server_details == NULL, "Eroare alocare memorie");
	server_details->sin_family = AF_INET;
	server_details->sin_port = htons((uint16_t)port);
	server_details->sin_addr.s_addr = INADDR_ANY;
	// Am completat detaliile header-ului de socket
	// pentru ca acest program sa fie recunoscut ca server
	int chk_ret;
	chk_ret = bind(tcp_sock_descr,
				   (struct sockaddr *)server_details,
				   SOCKADDR_IN_SIZE);
	DIE(chk_ret < 0, "Bind-ul nu s-a reusit");
	// Socket-ului i-a fost asociat un port
	chk_ret = listen(tcp_sock_descr, MAX_PENDING_CONNECTIONS);
	DIE(chk_ret < 0, "Configurarea socket-ului pentru ascultare esuata");
	// Serverul poate "asculta" cererile clientilor acum
	fd_set *inputs = malloc(FD_SET_SIZE);
	DIE(inputs == NULL, "Eroare alocare memorie");
	fd_set *prev_inputs = malloc(FD_SET_SIZE);
	DIE(prev_inputs == NULL, "Eroare alocare memorie");
	int max_input_rank;
	FD_ZERO(inputs);
	FD_ZERO(prev_inputs);
	FD_SET(tcp_sock_descr, inputs);
	max_input_rank = tcp_sock_descr;
	// Am initializat multimea de surse din care se poate citi
	struct TCPClientsDB *tcp_clients = init_clients_db();
	// Am initializat baza de date a clientilor
	while (TRUE_VAL) {
		memcpy(prev_inputs, inputs, FD_SET_SIZE);

		chk_ret = select(max_input_rank + 1, prev_inputs, NULL, NULL, NULL);
		DIE(chk_ret < 0, "Eroare la selectia sursei de input");

		for (int i = 0; i <= max_input_rank; i++) {
			if (FD_ISSET(i, prev_inputs)) {
				control_inp_srcs(inputs, prev_inputs,
								 &max_input_rank, i,
								 tcp_sock_descr);
			}
		}
	}
	close(tcp_sock_descr);
	// Socketul serverului a fost inchis
	return 0;
}
