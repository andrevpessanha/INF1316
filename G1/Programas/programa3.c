#include <stdlib.h>
#include <stdio.h>

int main (void){
	int i, j, k = 0;

	while(1){
		for(i = 0; i < 1000; i++)
			for(j = 0; j < 1000; j++) {

				k += i + j;
				//printf("PROGRAMA 3: k = %d\n",k);
			}

	}

}
