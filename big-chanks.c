#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "mtmm.h"

int main(){

	int i;
	for (i = 0; i < 1000; i++)
		if (malloc(700000) == 0){
			fprintf(stderr, "big chunck test FAILED\n");
			return (1);
		}

	fprintf(stdout, "big chunck test SUCCEEDED\n");
	return 0;
}
