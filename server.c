#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "tools.h"

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	DIE (argc < 2, "Argumente insuficiente pentru server");

	int socket_descr;
	socket_descr = socket(AF_INET, SOCK_STREAM, 0);
	DIE(socket_descr < 0, "Nu s-a putut initializa socketul");
	// Am creat socket-ul serverului
	disable_neagle_algorithm(socket_descr);
	// Am dezactivat algoritmul lui Neagle
	int port = atoi(argv[1]);
	DIE((port > MAX_PORT || port < MIN_PORT), "Port invalid");
	// Am parsat ca numar portul dat ca parametru programului
	struct sockaddr_in *server_details;
	server_details = calloc(1, SOCKADDR_IN_SIZE);
	server_details.sin_family = AF_INET;
	server_details.sin_port = htons((uint16_t)port);
	server_details.sin_addr.s_addr = INADDR_ANY;
	// Am completat detaliile header-ului de socket
	// pentru ca acest program sa fie recunoscut ca server
	int chk_ret;
	chk_ret = bind(socket_descr,
				   (struct sockaddr *)server_details,
				   SOCKADDR_IN_SIZE);
	DIE(chk_ret < 0, "Bind-ul nu s-a reusit");
	// Socket-ului i-a fost asociat un port
	chk_ret = listen(socket_descr, MAX_PENDING_CONNECTIONS);
	DIE(chk_ret < 0, "Configurarea socket-ului pentru ascultare esuata");
	// Serverul poate "asculta" cererile clientilor acum
	
	close(socket_descr);
	// Socketul a fost inchis
	return 0;
}