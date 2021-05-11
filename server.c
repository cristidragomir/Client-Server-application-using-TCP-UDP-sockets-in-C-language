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
	// Atunci cand pe server se executa comanda "exit"
	// toti clientii trebuie sa fie deconectati automat
	char *response = calloc(PAYLOAD_LEN, CHAR_SIZE);
	DIE(response == NULL, "Eroare alocare memorie");
	response[0] = 'E';
	for (uint32_t i = 0; i < tcp_clients->cnt; i++) {
		if (tcp_clients->cls[i].socket >= 0) {
			int chk_func;
			chk_func = send(tcp_clients->cls[i].socket, 
				response, PAYLOAD_LEN, 0);
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
	// Un client se conecteaza cu ID-ul altui client deja conectat
	printf("Client %s already connected.\n", sock_id_corresp);
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
	// S-a primit un mesaj TCP de tip '1' ceea ce inseamna ca trebuie
	// completate informatiile despre un client
	uint32_t cl_index = find_client_by_socket(inp_index, tcp_clients);
	// Se cauta indexul clientului al carui id trebuie completat
	char *sock_id_corresp = rec_msg->client_id;
	// Se preia id-ul trimis in buffer
	uint32_t cl_id_len = strlen(sock_id_corresp);
	for (uint32_t i = 0; i < tcp_clients->cnt; i++) {
		if (cl_index != i && cl_id_len == strlen(tcp_clients->cls[i].id) &&
			!memcmp(tcp_clients->cls[i].id, sock_id_corresp, cl_id_len)) {
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
				// Se trimit mesajele restante cat timp clientul a fost offline
				while(tcp_clients->cls[i].prev_msgs != NULL && 
					!queue_empty(tcp_clients->cls[i].prev_msgs)) {
					char *prev_buff = queue_deq(tcp_clients->cls[i].prev_msgs);
					int chk_func = send(tcp_clients->cls[i].socket, prev_buff, 
						PAYLOAD_LEN, 0);
					DIE(chk_func < 0, "Eroare trimitere informatii");
				}
				tcp_clients->cls[i].prev_msgs = NULL;
				struct in_addr aux;
				aux.s_addr = tcp_clients->cls[i].ip;
				printf("New client %s connected from %s:%hu.\n", 
					tcp_clients->cls[i].id, inet_ntoa(aux), 
					ntohs(tcp_clients->cls[i].port));
			}
			return;
		}
	}
	memcpy(tcp_clients->cls[cl_index].id, 
		sock_id_corresp, strlen(sock_id_corresp));
	tcp_clients->cls[cl_index].id[cl_id_len] = '\0';
	struct in_addr aux;
	aux.s_addr = tcp_clients->cls[cl_index].ip;
	printf("New client %s connected from %s:%hu.\n", 
		tcp_clients->cls[cl_index].id,
		inet_ntoa(aux), ntohs(tcp_clients->cls[cl_index].port));
	// Se afiseaza detaliile de conectare
}

void print_topics_cls_list(list *topics_cls_list) {
	// Functie folosita in debugging - afiseaza toata tabela de dispersie
	// Cu topicurile si informatiile despre o abonare
	list curr = *topics_cls_list;
	while (curr) {
		printf("%s:", ((struct topics_cls *)curr->element)->topic);
		list subbers = ((struct topics_cls *)curr->element)->subs;
		while (subbers) {
			printf("%s %d|", 
				((struct Subscription *)subbers->element)->client->id,
				((struct Subscription *)subbers->element)->sf);
			subbers = subbers->next;
		}
		curr = curr->next;
		printf("\n");	
	}
	printf("\n");
}

int find_topic(struct TCPmsg *rec_msg, list *topics_cls_list, 
	list *curr, char sub_unsub) {
	// Se realizeaza o cautare a topic-ului in lista de chei
	// a tabelei de dispersie. Daca aceasta nu exista, se adauga
	// in lista
	*curr = *topics_cls_list;
	list next_el;
	if (*curr == NULL) {
		// Lista este goala
		if (sub_unsub == '1') {
			return -1;
		}
		*topics_cls_list = cons(malloc(sizeof(struct topics_cls)),
			*topics_cls_list);
		*curr = *topics_cls_list;
		((struct topics_cls *)((*curr)->element))->subs = NULL;
		memcpy(((struct topics_cls *)((*curr)->element))->topic,
			rec_msg->topic_to_sub_unsub, 
			strlen(rec_msg->topic_to_sub_unsub) + 1);
	} else {
		next_el = (*curr)->next;
		while (next_el != NULL) {
			if (strlen(((struct topics_cls *)((*curr)->element))->topic) ==
				strlen(rec_msg->topic_to_sub_unsub) &&
				!memcmp(((struct topics_cls *)((*curr)->element))->topic, 
				rec_msg->topic_to_sub_unsub, 
				strlen(rec_msg->topic_to_sub_unsub))) {
				break;
			}
			*curr = next_el;
			next_el = next_el->next;
		}
		// Se parcurge lista pana se gaseste (sau nu) elementul dorit
		if (next_el == NULL) {
			// Se verifica daca nu cumva ultimul element este cel cautat
			// In caz contrar ne folosim de acesta pentru a extinde lista
			if (strlen(((struct topics_cls *)((*curr)->element))->topic) !=
				strlen(rec_msg->topic_to_sub_unsub) ||
				memcmp(((struct topics_cls *)((*curr)->element))->topic, 
				rec_msg->topic_to_sub_unsub, 
				strlen(rec_msg->topic_to_sub_unsub))) {
				// Trebuie creat un nou topic la care clientii se pot abona
				if (sub_unsub == '1') {
					return -1;
				}
				*topics_cls_list = cons(malloc(sizeof(struct topics_cls)),
					*topics_cls_list);
				*curr = *topics_cls_list;
				((struct topics_cls *)((*curr)->element))->subs = NULL;
				memcpy(((struct topics_cls *)((*curr)->element))->topic,
					rec_msg->topic_to_sub_unsub,
					strlen(rec_msg->topic_to_sub_unsub) + 1);
			}
		}
	}
	return 1;
}

void remove_subscription(struct TCPmsg *rec_msg, int inp_index, 
	struct TCPClientsDB *tcp_clients, list *topics_cls_list) {
	// Stergem o anumita intrare din tabela de dispersie
	uint32_t cl_index = find_client_by_socket(inp_index, tcp_clients);
	list curr;
	int chk_func = find_topic(rec_msg, topics_cls_list, &curr, '1');
	if (chk_func == -1) {
		#if 0
		printf("Topicul pentru dezabonare nu a fost gasit!\n");
		#endif
		return;
	}
	list prev, act_sub;
	// act_sub - pointer catre o structura ce 
	// defineste abonamentul actual evaluat in lista
	prev = act_sub = ((struct topics_cls *)curr->element)->subs;
	while (act_sub->next != NULL) {
		if (strlen((((struct Subscription *)act_sub->element)->client)->id) ==
			strlen(tcp_clients->cls[cl_index].id) &&
			!memcmp((((struct Subscription *)act_sub->element)->client)->id,
				tcp_clients->cls[cl_index].id, 
				strlen(tcp_clients->cls[cl_index].id))) {
			break;
		}
		prev = act_sub;
		act_sub = act_sub->next;
	}
	if (act_sub->next == NULL) {
		if (strlen((((struct Subscription *)act_sub->element)->client)->id) !=
			strlen(tcp_clients->cls[cl_index].id) ||
			memcmp((((struct Subscription *)act_sub->element)->client)->id, 
				tcp_clients->cls[cl_index].id, 
				strlen(tcp_clients->cls[cl_index].id))) {
			#if 0
			printf("Clientul nu este abonat la acest topic!\n");
			#endif
			return;
		}
	}
	if (prev == act_sub) {
		((struct topics_cls *)curr->element)->subs = act_sub->next;
	} else {
		prev->next = act_sub->next;
	}
	free(act_sub);
	#if 0
	print_topics_cls_list(topics_cls_list);
	#endif
}

void add_subscription(struct TCPmsg *rec_msg, int inp_index, 
	struct TCPClientsDB *tcp_clients, list *topics_cls_list) {
	// Adaugam la tabela de dispersie, la o anumita cheie(nume topic),
	// un nou client(struct Client_info)
	uint32_t cl_index = find_client_by_socket(inp_index, tcp_clients);
	list curr;
	// Incercam sa cautam topicul sau sa adaugam inca o intrare in lista
	// de chei din tabela de dispersie
	find_topic(rec_msg, topics_cls_list, &curr, '0');
	// Se verifica daca la topicul respectiv clientul este deja abonat
	// In acest caz, se schimba doar facilitatea de store-and-forward
	list aux_subs = ((struct topics_cls *)(curr->element))->subs;
	if (aux_subs == NULL) {
		// Nu exista niciun abonat la acest topic si se adauga unul
		struct Subscription *new_subscription;
		new_subscription = malloc(sizeof(struct Subscription));
		DIE(new_subscription == NULL, "Eroare alocare memorie");
		new_subscription->client = &(tcp_clients->cls[cl_index]);
		new_subscription->sf = (int)(rec_msg->sf) - '0';
		new_subscription->client->prev_msgs = NULL;
		((struct topics_cls *)(curr->element))->subs = 
			cons(new_subscription, 
				((struct topics_cls *)(curr->element))->subs);
	} else {
		list aux_subs_next = aux_subs->next;
		while(aux_subs_next != NULL) {
			struct Subscription *subscription; 
			subscription = ((struct Subscription *)aux_subs->element);
			if (strlen((subscription->client)->id) ==
				strlen(tcp_clients->cls[cl_index].id) &&
				!memcmp((subscription->client)->id,
					tcp_clients->cls[cl_index].id, 
					strlen(tcp_clients->cls[cl_index].id))) {
				break;
			}
			aux_subs = aux_subs_next;
			aux_subs_next = aux_subs->next;
		}
		// S-a cerceteaza daca un client este deja abonat la acest topic
		struct Subscription *subscription; 
		subscription = ((struct Subscription *)aux_subs->element);
		if (aux_subs_next == NULL) {
			// Se verifica daca ultimul abonament este cel facut de clientul
			// care trimite cererea
			struct Subscription *subscription; 
			subscription = ((struct Subscription *)aux_subs->element);
			if (strlen((subscription->client)->id) !=
				strlen(tcp_clients->cls[cl_index].id) ||
				memcmp((subscription->client)->id,
					tcp_clients->cls[cl_index].id, 
					strlen(tcp_clients->cls[cl_index].id))) {
				// Se adauga un nou abonament la lista de abonamente la topic
				struct Subscription *new_subscription;
				new_subscription = malloc(sizeof(struct Subscription));
				DIE(new_subscription == NULL, "Eroare alocare memorie");
				new_subscription->client = &(tcp_clients->cls[cl_index]);
				new_subscription->sf = (int)(rec_msg->sf) - '0';
				new_subscription->client->prev_msgs = NULL;
				((struct topics_cls *)(curr->element))->subs = 
					cons(new_subscription, 
						((struct topics_cls *)(curr->element))->subs);
			} else {
				// Abonamentul existent al clientului pe topic este reinnoit
				subscription->sf = (int)(rec_msg->sf) - '0';
			}
		} else {
			// Abonamentul existent al clientului pe topic este reinnoit
			subscription->sf = (int)(rec_msg->sf) - '0';
		}
	}
	#if 0
	print_topics_cls_list(topics_cls_list);
	#endif
}

void share_udp_msg(char *to_share, struct UDPmsg *recv_msg, 
	list *topics_cls_list) {
	list curr;
	struct TCPmsg aux;
	memcpy(aux.topic_to_sub_unsub, 
		recv_msg->topic, strlen(recv_msg->topic) + 1);
	int chk_func = find_topic(&aux, topics_cls_list, &curr, '1');
	if (chk_func == -1) {
		#if 0
		printf("Topicul la care s-a primit mesaj UDP nu contine abonati!\n");
		#endif
		return;
	}
	list subber = ((struct topics_cls *)curr->element)->subs;
	while (subber) {
		// Se parcurge lista de abonamente la acest topic
		// si se trimite sau se salveaza mesajul abonatilor interesati
		struct Client_info *curr_client;
		curr_client = ((struct Subscription *)subber->element)->client;
		if (curr_client->is_connected == '1') {
			chk_func = send(curr_client->socket, to_share, PAYLOAD_LEN, 0);
			DIE(chk_func < 0, "Eroare trimitere informatii");
		} else if(curr_client->is_connected == '0' 
			&& ((struct Subscription *)subber->element)->sf == 1) {
			if (curr_client->prev_msgs == NULL) {
				curr_client->prev_msgs = queue_create();
			}
			char *to_share_copy = malloc(PAYLOAD_LEN * CHAR_SIZE);
			DIE(to_share_copy == NULL, "Eroare alocare memorie");
			memcpy(to_share_copy, to_share, PAYLOAD_LEN * CHAR_SIZE);
			queue_enq(curr_client->prev_msgs, to_share_copy);
		}
		subber = subber->next;
	}
}

void handle_udp_msg(int inp_index, char *buffer, list *topics_cls_list) {
	// Cand se primeste un mesaj UDP acesta trebuie forwardat
	// catre clientii TCP care sunt abonati pe topic-ul respectiv
	struct sockaddr_in sender_details;
	char *to_share = malloc(PAYLOAD_LEN * CHAR_SIZE);
	DIE(to_share == NULL, "Eroare alocare memorie");
	unsigned int *sender_details_len = malloc(sizeof(unsigned int));
	DIE(sender_details_len == NULL, "Eroare alocare memorie");
	recvfrom(inp_index, buffer, PAYLOAD_LEN, 0,
		(struct sockaddr *) &sender_details, sender_details_len);
	to_share[0] = '4';
	int udp_msg_len = TOPIC_LEN + CHAR_SIZE + CONTENT_LEN;
	memcpy(to_share + CHAR_SIZE, buffer, udp_msg_len);
	memcpy(to_share + CHAR_SIZE + udp_msg_len,
		&sender_details.sin_addr.s_addr, 4 * CHAR_SIZE);
	memcpy(to_share + CHAR_SIZE + udp_msg_len + 4 * CHAR_SIZE, 
		&sender_details.sin_port, 2 * CHAR_SIZE);
	struct UDPmsg recv_msg;
	DIE(parse_message_udp(&recv_msg, buffer) < 0, "Eroare parsare mesaj");
	// Structura buffer: tip mesaj TCP = 4, mesajul UDP, 4 bytes pentru IP
	// si 2 bytes pentru port
	share_udp_msg(to_share, &recv_msg, topics_cls_list);
}

void add_new_client(struct TCPClientsDB *tcp_clients, int new_sock,
	struct sockaddr_in *incoming_client) {
	// Adaugam un nou client in baza de date
	int index = tcp_clients->cnt;
	tcp_clients->cls[index].id[10] = '\0';
	tcp_clients->cls[index].is_connected = '1';
	tcp_clients->cls[index].socket = new_sock;
	tcp_clients->cls[index].port = incoming_client->sin_port;
	tcp_clients->cls[index].ip = incoming_client->sin_addr.s_addr;
	tcp_clients->cnt++;
}

void control_inp_srcs(fd_set *inputs, int *max_input_rank, int inp_index,
	int server_tcp_socket, int server_udp_socket,
	struct TCPClientsDB *tcp_clients, list *topics_cls_list) {
	// Aici se manipuleaza ceea ce ajunge la server pe orice intrare
	int new_sock, chk_func;
	char *buffer = calloc(PAYLOAD_LEN, CHAR_SIZE);
	DIE (buffer == NULL, "Eroare alocare memorie");
	struct sockaddr_in *incoming_client = malloc(SOCKADDR_IN_SIZE);
	DIE(incoming_client == NULL, "Eroare alocare memorie");
	if (inp_index == server_tcp_socket) {
		// In acest caz incercam sa primim o noua conexiune catre server
		socklen_t inc_client_size = SOCKADDR_IN_SIZE;
		new_sock = accept(server_tcp_socket, 
			(struct sockaddr *)incoming_client, &inc_client_size);
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
		add_new_client(tcp_clients, new_sock, incoming_client);
		// S-a adaugat un nou client in baza noastra de date
	} else if (inp_index == server_udp_socket) {
		// In acest caz se primesc informatii de la clientii UDP
		handle_udp_msg(inp_index, buffer, topics_cls_list);
		free(buffer);
		return;
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
			printf("Client %s disconnected.\n", tcp_clients->cls[cl_index].id);
			tcp_clients->cls[cl_index].socket = -1;
			tcp_clients->cls[cl_index].is_connected = '0';
			close(inp_index);
			FD_CLR(inp_index, inputs);
		} else {
			// Clientul TCP trimite informatii catre server
			struct TCPmsg rec_msg;
			DIE(parse_message_tcp(&rec_msg, buffer) < 0, 
				"Eroare de parsare a bufferului");
			if (rec_msg.type == '1') {
				// Un client TCP si-a trimis ID-ul
				recv_cl_id(inp_index, tcp_clients, &rec_msg, inputs);
			} else if (rec_msg.type == '2') {
				// Un client TCP a trimis o cerere de subscribe
				add_subscription(&rec_msg, inp_index,
					tcp_clients, topics_cls_list);
			} else if (rec_msg.type == '3') {
				// Un client TCP a trimit o cerere de unsubscribe
				remove_subscription(&rec_msg, inp_index,
					tcp_clients, topics_cls_list);
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
	if (setsockopt(*socket, SOL_SOCKET, SO_REUSEADDR, 
			&enable, sizeof(int)) == -1) {
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
		DIE(chk_ret < 0, 
			"Configurarea socket-ului TCP pentru ascultare esuata");
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
	int udp_sock_descr) {
	// Se elibereaza resursele utilizate de program
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
	configure_fd_sets(udp_sock_descr, tcp_sock_descr, &inputs,
		&prev_inputs, &max_input_rank);
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
			DIE(buffer == NULL, "Eroare alocare memorie");
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
				control_inp_srcs(inputs, &max_input_rank, i,
								tcp_sock_descr, udp_sock_descr, tcp_clients,
								&topics_cls_list);
			}
		}	
	}
	free_mem(tcp_clients, inputs, prev_inputs, tcp_sock_descr, udp_sock_descr);
	// Socketul serverului a fost inchis
	return 0;
}
