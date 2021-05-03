#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "tools.h"

void duplicate_id(char *sock_id_corresp, int inp_index, 
	struct TCPClientsDB *tcp_clients, fd_set *inputs) {
	
	printf("Client %s already connected\n", sock_id_corresp);
	tcp_clients->cnt--;
	char *response = calloc(PAYLOAD_LEN, sizeof(char));
	DIE(response == NULL, "Eroare alocare memorie");
	response[0] = 'E';
	int chk_func;
	chk_func = send(inp_index, response, PAYLOAD_LEN, 0);
	DIE(chk_func < 0, "Eroare trimitere informatii");
	close(inp_index);
	FD_CLR(inp_index, inputs);
}

void recv_cl_id(int inp_index, struct TCPClientsDB *tcp_clients, 
	char *buffer, fd_set *inputs) {
	uint32_t cl_index;
	for (uint32_t i = 0; i < tcp_clients->cnt; i++) {
		if (inp_index == tcp_clients->cls[i].socket) {
			cl_index = i;
			break;
		}
	}
	char *sock_id_corresp = buffer + sizeof(char);
	// Se preia id-ul trimis in buffer
	uint32_t cl_id_len = strlen(sock_id_corresp);
	for (uint32_t i = 0; i < tcp_clients->cnt; i++) {
		if (cl_index != i && 
			memcmp(tcp_clients->cls[i].id, 
				sock_id_corresp, cl_id_len) == 0) {
			
			duplicate_id(sock_id_corresp, inp_index, tcp_clients, inputs);
			return;
		}
	}
	memcpy(tcp_clients->cls[cl_index].id, 
		sock_id_corresp, strlen(sock_id_corresp));
	tcp_clients->cls[cl_index].id[cl_id_len] = '\0';
	struct in_addr aux;
	aux.s_addr = tcp_clients->cls[cl_index].ip;
	printf("New client %s connected from %s:%hu\n", tcp_clients->cls[cl_index].id, 
		inet_ntoa(aux), ntohs(tcp_clients->cls[cl_index].port));
	// Se afiseaza detaliile de conectare
}

void control_inp_srcs(fd_set *inputs, 
	int *max_input_rank, int inp_index, 
	int server_socket, struct TCPClientsDB *tcp_clients) {

	int new_sock, chk_func;
	char *buffer = calloc(PAYLOAD_LEN, sizeof(char));
	struct sockaddr_in *incoming_client = malloc(SOCKADDR_IN_SIZE);
	DIE(incoming_client == NULL, "Eroare alocare memorie");
	if (inp_index == server_socket) {
		// In acest caz incercam sa primim o noua conexiune catre server
		socklen_t inc_client_size = SOCKADDR_IN_SIZE;
		new_sock = accept(server_socket, (struct sockaddr *)incoming_client, &inc_client_size);
		DIE(new_sock < 0, "Eroare acceptare conexiune noua");
		// Am acceptat o noua conexiune la acest server
		FD_SET(new_sock, inputs);
		if (new_sock > *max_input_rank) {
			*max_input_rank = new_sock;
		}
		// Am introdus in multimea de surse de citire o noua sursa
		for (int j = 0; j < ID_CLIENT_LEN; j++) {
			tcp_clients->cls[tcp_clients->cnt].id[j] = '-';
		}
		tcp_clients->cls[tcp_clients->cnt].id[10] = '\0';
		tcp_clients->cls[tcp_clients->cnt].is_connected = '1';
		tcp_clients->cls[tcp_clients->cnt].socket = new_sock;
		tcp_clients->cls[tcp_clients->cnt].port = incoming_client->sin_port;
		tcp_clients->cls[tcp_clients->cnt].ip = incoming_client->sin_addr.s_addr;
		tcp_clients->cnt++;
	} else {
		// De la o conexiune deja stabilita primim informatii
		chk_func = recv(inp_index, buffer, PAYLOAD_LEN, 0);
		DIE(chk_func < 0, "Eroare receptie informatii");
		if (chk_func == 0) {
			// Clientul a incheiat conexiunea cu serverul
			uint32_t cl_index;
			for (uint32_t i = 0; i < tcp_clients->cnt; i++) {
				if (inp_index == tcp_clients->cls[i].socket) {
					cl_index = i;
					break;
				}
			}
			printf("Client %s disconnected\n", tcp_clients->cls[cl_index].id);
			tcp_clients->cls[cl_index].is_connected = '0';
			close(inp_index);
			FD_CLR(inp_index, inputs);
		} else {
			// Clientul trimite informatii catre server
			if (buffer[0] == '1') {
				recv_cl_id(inp_index, tcp_clients, buffer, inputs);
			}
		}
	}
	free(buffer);
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
		if (FD_ISSET(i, prev_inputs)) {
			char *buffer = malloc(PAYLOAD_LEN * sizeof(char));
			fgets(buffer, PAYLOAD_LEN, stdin);
			if (strncmp(buffer, "exit", 4) == 0) {
				disconnect_all_cls(tcp_cls);
				break;
			}
			free(buffer);
		}
		for (int i = 1; i <= max_input_rank; i++) {
			if (FD_ISSET(i, prev_inputs)) {
				control_inp_srcs(inputs, &max_input_rank, i,
								 tcp_sock_descr, tcp_clients);
			}
		}
	}
	close(tcp_sock_descr);
	// Socketul serverului a fost inchis
	return 0;
}
