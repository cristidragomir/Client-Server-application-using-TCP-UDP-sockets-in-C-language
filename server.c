#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "tools.h"
#include "list.h"
#include "queue.h"

void disconnect_all_cls(struct TCPClientsDB *tcp_clients, fd_set *inputs) {
	char *response = calloc(PAYLOAD_LEN, CHAR_SIZE);
	DIE(response == NULL, "Eroare alocare memorie");
	response[0] = 'E';
	for (uint32_t i = 0; i < tcp_clients->cnt; i++) {
		if (tcp_clients->cls[i].socket >= 0) {
			int chk_func;
			chk_func = send(tcp_clients->cls[i].socket, response, PAYLOAD_LEN, 0);
			DIE(chk_func < 0, "Eroare trimitere informatii");
			close(tcp_clients->cls[i].socket);
			FD_CLR(i, inputs);
			tcp_clients->cls[i].is_connected = '0';
		}
	}
	free(response);
}

void duplicate_id(char *sock_id_corresp, int inp_index, 
	struct TCPClientsDB *tcp_clients, fd_set *inputs) {
	// Un potential nou client se conecteaza cu ID-ul altui client deja conectat
	printf("Client %s already connected\n", sock_id_corresp);
	tcp_clients->cnt--;
	char *response = calloc(PAYLOAD_LEN, CHAR_SIZE);
	DIE(response == NULL, "Eroare alocare memorie");
	response[0] = 'E';
	int chk_func;
	chk_func = send(inp_index, response, PAYLOAD_LEN, 0);
	DIE(chk_func < 0, "Eroare trimitere informatii");
	close(inp_index);
	FD_CLR(inp_index, inputs);
	free(response);
}

uint32_t find_client_by_socket(int inp_index, 
	struct TCPClientsDB *tcp_clients) {
	// Se cauta un anumit client dupa socket-ul TCP specific
	uint32_t cl_index;
	for (uint32_t i = 0; i < tcp_clients->cnt; i++) {
		if (inp_index == tcp_clients->cls[i].socket) {
			cl_index = i;
			break;
		}
	}
	return cl_index;
}

void recv_cl_id(int inp_index, struct TCPClientsDB *tcp_clients, 
	struct TCPmsg *rec_msg, fd_set *inputs) {
	uint32_t cl_index = find_client_by_socket(inp_index, tcp_clients);
	// Se cauta indexul clientului al carui id trebuie completat
	char *sock_id_corresp = rec_msg->client_id;
	// Se preia id-ul trimis in buffer
	uint32_t cl_id_len = strlen(sock_id_corresp);
	for (uint32_t i = 0; i < tcp_clients->cnt; i++) {
		if (cl_index != i && 
			memcmp(tcp_clients->cls[i].id, sock_id_corresp, cl_id_len) == 0) {
			if (tcp_clients->cls[i].is_connected == '1') {
				// Un alt client incearca sa se conecteze cu acelasi ID
				// al altui client deja conectat
				duplicate_id(sock_id_corresp, inp_index, tcp_clients, inputs);
			} else {
				// Clientul s-a conectat o data pe server 
				// si incearca sa se reconecteze
				tcp_clients->cnt--;
				tcp_clients->cls[i].is_connected = '1';
				tcp_clients->cls[i].socket = inp_index;
				tcp_clients->cls[i].port = tcp_clients->cls[cl_index].port;
				tcp_clients->cls[i].ip = tcp_clients->cls[cl_index].ip;
			}
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

void print_topics_cls_list(list *topics_cls_list) {
	list curr = *topics_cls_list;
	while (curr) {
		printf("%s:", ((struct topics_cls *)curr->element)->topic);
		list subbers = ((struct topics_cls *)curr->element)->subs;
		while (subbers) {
			printf("%s %d|", ((struct Subscription *)subbers->element)->client->id, ((struct Subscription *)subbers->element)->sf);
			subbers = subbers->next;
		}
		curr = curr->next;
		printf("\n");	
	}
	printf("\n");
}

void find_topic(struct TCPmsg *rec_msg, list *topics_cls_list, list *curr, list *topics_cls_list) {
	*curr = *topics_cls_list;
	list next_el;
	if (curr == NULL) {
		*topics_cls_list = cons(malloc(sizeof(struct topics_cls)), *topics_cls_list);
		*curr = *topics_cls_list;
		((struct topics_cls *)((*curr)->element))->subs = NULL;
		memcpy(((struct topics_cls *)((*curr)->element))->topic,
			rec_msg->topic_to_sub_unsub, 
			strlen(rec_msg->topic_to_sub_unsub) + 1);
	} else {
		next_el = (*curr)->next;
		while (next_el != NULL) {
			if (!memcmp(((struct topics_cls *)((*curr)->element))->topic, 
				rec_msg->topic_to_sub_unsub, 
				strlen(rec_msg->topic_to_sub_unsub))) {
				break;
			}
			*curr = next_el;
			next_el = next_el->next;
		}
		if (next_el == NULL) {
			if (memcmp(((struct topics_cls *)(curr->element))->topic, 
				rec_msg->topic_to_sub_unsub, 
				strlen(rec_msg->topic_to_sub_unsub))) {
				// Trebuie creat un nou topic la care clientii se pot abona
				*topics_cls_list = cons(malloc(sizeof(struct topics_cls)), *topics_cls_list);
				*curr = *topics_cls_list;
				((struct topics_cls *)(curr->element))->subs = NULL;
				memcpy(((struct topics_cls *)(curr->element))->topic,
					rec_msg->topic_to_sub_unsub,
					strlen(rec_msg->topic_to_sub_unsub) + 1);
			}
		}
	}
}

void remove_subscription(struct TCPmsg *rec_msg, int inp_index,
	struct TCPClientsDB *tcp_clients) {
	// Stergem o anumita intrare din tabela de dispersie
	uint32_t cl_index = find_client_by_socket(inp_index, tcp_clients);
	list curr, next_el;
}

void add_subscription(struct TCPmsg *rec_msg, int inp_index, 
	struct TCPClientsDB *tcp_clients, list *topics_cls_list) {
	// Adaugam la tabela de dispersie, la o anumita cheie(nume topic), un nou client(struct Client_info)
	uint32_t cl_index = find_client_by_socket(inp_index, tcp_clients);
	list curr, next_el;
	find_topic()
	// Se verifica daca la topicul respectiv clientul este deja abonat
	// In acest caz, se schimba doar facilitatea de store-and-forward
	list aux_subs = ((struct topics_cls *)(curr->element))->subs;
	if (aux_subs == NULL) {
		struct Subscription *new_subscription = malloc(sizeof(struct Subscription));
		new_subscription->client = &(tcp_clients->cls[cl_index]);
		new_subscription->sf = (int)(rec_msg->sf) - '0';
		new_subscription->client->prev_msgs = NULL;
		((struct topics_cls *)(curr->element))->subs = 
			cons(new_subscription, ((struct topics_cls *)(curr->element))->subs);
	} else {
		list aux_subs_next = aux_subs->next;
		while(aux_subs_next != NULL) {
			if (!memcmp((((struct Subscription *)aux_subs->element)->client)->id,
				tcp_clients->cls[cl_index].id, strlen(tcp_clients->cls[cl_index].id))) {
				break;
			}
			aux_subs = aux_subs_next;
			aux_subs_next = aux_subs->next;
		}
		if (aux_subs_next == NULL) {
			if (memcmp((((struct Subscription *)aux_subs->element)->client)->id,
				tcp_clients->cls[cl_index].id, strlen(tcp_clients->cls[cl_index].id))) {
				
				struct Subscription *new_subscription = malloc(sizeof(struct Subscription));
				new_subscription->client = &(tcp_clients->cls[cl_index]);
				new_subscription->sf = (int)(rec_msg->sf) - '0';
				new_subscription->client->prev_msgs = NULL;
				((struct topics_cls *)(curr->element))->subs = 
					cons(new_subscription, ((struct topics_cls *)(curr->element))->subs);
			} else {
				((struct Subscription *)aux_subs->element)->sf = (int)(rec_msg->sf) - '0';
			}
		} else {
			((struct Subscription *)aux_subs->element)->sf = (int)(rec_msg->sf) - '0';
		}
	}
	print_topics_cls_list(topics_cls_list);
}

void control_inp_srcs_tcp(fd_set *inputs, 
	int *max_input_rank, int inp_index, 
	int server_tcp_socket, int server_udp_socket, 
	struct TCPClientsDB *tcp_clients, list *topics_cls_list) {

	int new_sock, chk_func;
	char *buffer = calloc(PAYLOAD_LEN, CHAR_SIZE);
	struct sockaddr_in *incoming_client = malloc(SOCKADDR_IN_SIZE);
	DIE(incoming_client == NULL, "Eroare alocare memorie");
	if (inp_index == server_tcp_socket) {
		// In acest caz incercam sa primim o noua conexiune catre server
		socklen_t inc_client_size = SOCKADDR_IN_SIZE;
		new_sock = accept(server_tcp_socket, (struct sockaddr *)incoming_client, &inc_client_size);
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
		// S-a adaugat un nou client in baza noastra de date
	} else if (inp_index == server_udp_socket) {
		// In acest caz se primesc informatii de la clientii UDP
		struct sockaddr_in sender_details;
		unsigned int *sender_details_len = malloc(sizeof(unsigned int));
		chk_func = recvfrom(inp_index, buffer, PAYLOAD_LEN, 0,
			(struct sockaddr *) &sender_details, sender_details_len);
		struct UDPmsg recv_msg;
		DIE(parse_message_udp(&recv_msg, buffer) < 0, "Eroare parsare mesaj");
		display_udp_msg(&recv_msg, &sender_details);
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
			tcp_clients->cls[cl_index].socket = -1;
			tcp_clients->cls[cl_index].is_connected = '0';
			close(inp_index);
			FD_CLR(inp_index, inputs);
		} else {
			// Clientul TCP trimite informatii catre server
			struct TCPmsg rec_msg;
			DIE(parse_message_tcp(&rec_msg, buffer) < 0, "Eroare de parsare a bufferului");
			if (rec_msg.type == '1') {
				// Un client TCP si-a trimis ID-ul
				recv_cl_id(inp_index, tcp_clients, &rec_msg, inputs);
			} else if (rec_msg.type == '2') {
				// Un client TCP a trimis o cerere de subscribe
				add_subscription(&rec_msg, inp_index, tcp_clients, topics_cls_list);
			} else if (rec_msg.type == '3') {
				// Un client TCP a trimit o cerere de unsubscribe
				display_type_3_msg(&rec_msg);
			}
		}
	}
	free(buffer);
}

void server_common_config(int *socket, char *port_arg, int tcp_sock_type) {
	if (tcp_sock_type == 1) {
		disable_neagle_algorithm(*socket);
		// Am dezactivat algoritmul lui Neagle
	}
	int port = atoi(port_arg);
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
	int enable = 1;
	if (setsockopt(*socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
		perror("Bind-ul nu s-a reusit");
		exit(1);
	}
	// Deblocam portul in scopul reutilizarii sale
	int chk_ret;
	chk_ret = bind(*socket, (struct sockaddr *)server_details, 
				SOCKADDR_IN_SIZE);
	DIE(chk_ret < 0, "Bind-ul nu s-a reusit");
	// Socket-ului i-a fost asociat un port
	if (tcp_sock_type == 1) {
		chk_ret = listen(*socket, MAX_PENDING_CONNECTIONS);
		DIE(chk_ret < 0, "Configurarea socket-ului TCP pentru ascultare esuata");
		// Serverul poate "asculta" cererile clientilor acum
	}
}

void configure_server_tcp(int *tcp_sock_descr, char *port_arg) {
	*tcp_sock_descr = socket(AF_INET, SOCK_STREAM, 0);
	DIE(*tcp_sock_descr < 0, "Nu s-a putut initializa socket-ul TCP");
	// Am creat socket-ul TCP al serverului
	server_common_config(tcp_sock_descr, port_arg, 1);
}

void configure_server_udp(int *udp_sock_descr, char *port_arg) {
	*udp_sock_descr = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(*udp_sock_descr < 0, "Nu s-a putut initializa socket-ul UDP");
	// Am creat socket-ul UDP al serverului
	server_common_config(udp_sock_descr, port_arg, 0);
}

void configure_fd_sets(int socket1, int socket2, fd_set **inputs,
	fd_set **prev_inputs, int *max_input_rank) {
	// Se configureaza multimea de descriptori de input
	*inputs = malloc(FD_SET_SIZE);
	DIE(*inputs == NULL, "Eroare alocare memorie");
	*prev_inputs = malloc(FD_SET_SIZE);
	DIE(*prev_inputs == NULL, "Eroare alocare memorie");
	FD_ZERO(*inputs);
	FD_ZERO(*prev_inputs);
	FD_SET(socket1, *inputs);
	FD_SET(socket2, *inputs);
	FD_SET(0, *inputs);
	if (socket1 > socket2) {
		*max_input_rank = socket1;
	} else {
		*max_input_rank = socket2;
	}
}

void free_mem(struct TCPClientsDB *tcp_clients,
	fd_set *inputs, fd_set *prev_inputs, int tcp_sock_descr,
	// Se elibereaza resursele utilizate de program
	int udp_sock_descr) {
	free(tcp_clients->cls);
	free(tcp_clients);
	free(inputs);
	free(prev_inputs);
	close(tcp_sock_descr);
	close(udp_sock_descr);
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	DIE (argc < 2, "Argumente insuficiente pentru server");
	int tcp_sock_descr, udp_sock_descr;
	configure_server_tcp(&tcp_sock_descr, argv[1]);
	configure_server_udp(&udp_sock_descr, argv[1]);
	// Configurare server pentru TCP si UDP
	fd_set *inputs, *prev_inputs;
	int max_input_rank;
	configure_fd_sets(udp_sock_descr, tcp_sock_descr, &inputs, &prev_inputs, &max_input_rank);
	// Am initializat multimea de surse din care se poate citi
	struct TCPClientsDB *tcp_clients = init_clients_db();
	// Am initializat baza de date a clientilor
	list topics_cls_list = NULL;
	int chk_ret;
	while (TRUE_VAL) {
		memcpy(prev_inputs, inputs, FD_SET_SIZE);
		// Retinerea multimii de surse de citire initial
		chk_ret = select(max_input_rank + 1, prev_inputs, NULL, NULL, NULL);
		DIE(chk_ret < 0, "Eroare la selectia sursei de input");
		if (FD_ISSET(0, prev_inputs)) {
			// Serverul citeste informatii de la tastatura
			char *buffer = calloc(PAYLOAD_LEN, CHAR_SIZE);
			fgets(buffer, PAYLOAD_LEN - 1, stdin);
			if (strncmp(buffer, "exit", 4) == 0) {
				disconnect_all_cls(tcp_clients, inputs);
				break;
			}
			free(buffer);
		}
		for (int i = 1; i <= max_input_rank; i++) {
			if (FD_ISSET(i, prev_inputs)) {
			// Serverul citeste informatii dintr-o anumita sursa
				control_inp_srcs_tcp(inputs, &max_input_rank, i,
								 tcp_sock_descr, udp_sock_descr, tcp_clients, &topics_cls_list);
			}
		}	
	}
	free_mem(tcp_clients, inputs, prev_inputs, tcp_sock_descr, udp_sock_descr);
	// Socketul serverului a fost inchis
	return 0;
}
