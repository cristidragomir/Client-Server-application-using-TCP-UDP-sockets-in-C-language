#include <stdio.h>
#include <stdlib.h>

#include "aux_macro.h"

int main()
{
	FILE *in = fopen("blabla.txt","rt");
	DIE(in == NULL, "Fisierul nu a fost gasit");
	return 0;
}