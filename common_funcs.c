#include <stdio.h>
#include <stdlib.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

#include "aux_macro.h"

void disable_neagle_algorithm(int socket) {
	uint32_t on = 1;
	int disabling_status = setsockopt(socket,
							IPPROTO_TCP,
							TCP_NODELAY,
							(char *) &on,
							sizeof(uint32_t));
	DIE(disabling_status < 0, 
		"Algoritmul lui Neagle nu a putut fi dezactivat");
}
