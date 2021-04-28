#include <stdio.h>
#include <stdlib.h>

#define MAX_PORT 65535
#define MIN_PORT 1024
#define MAX_PENDING_CONNECTIONS 512
#define SOCKADDR_IN_SIZE sizeof(struct sockaddr_in)

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

void disable_neagle_algorithm(int socket);