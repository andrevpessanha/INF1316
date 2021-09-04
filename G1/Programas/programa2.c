// I/O BOUND (Envia sinal SIGUSR1)

#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char *argv[]) {
	int i = 0, pidPai;
	pidPai = atoi(argv[0]);

	while(1) {
		//printf("PROGRAMA 2: %d\n", i);
		sleep(1);
		kill(pidPai, SIGUSR1);
		i++;
	}

}
