#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

/*Estrutura com os dados do programa*/
struct infoPrograma {
    char nome[20]; /* Nome do programa */ 
    int indice;  /* índice do programa equivalente à ordem que foi lida */
}; 
typedef struct infoPrograma InfoPrograma ;

/*Remove uma substring do string principal*/
char *strremove(char *str, const char *sub) {
	char *p, *q, *r;
	if ((q = r = strstr(str, sub)) != NULL) {
    	size_t len = strlen(sub);
    	while ((r = strstr(p = r + len, sub)) != NULL) {
        	while (p < r)
            	*q++ = *p++;
    	}
    	while ((*q++ = *p++) != '\0')
        	continue;
	}
	return str;
}

int main (void) {

	FILE *arquivo;
	size_t len = 0;
	ssize_t read;
	int segmento;
	char *programa;
	int i = 0;
	InfoPrograma * programas;

	arquivo = fopen("exec.txt", "r");
	if (arquivo == NULL) {
		 printf("Erro abrindo o arquivo\n");
		 exit(1);    	 
	}
	 
	//Número máximo de programas é 20
	segmento = shmget (IPC_PRIVATE, 20 * sizeof(InfoPrograma), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

	programas = (InfoPrograma *) shmat (segmento, 0, 0);
	printf("Ponteiro: %p\n",segmento);
	 
	// lê do arquivo uma linha por vez
	while ((read = getline(&programa, &len, arquivo)) != -1) {
		
		programa = strremove(programa,"Run <");
		programa = strremove(programa,">");
		programa = strremove(programa,"\n");
		
		strcpy(programas[i].nome,programa);
		programas[i].indice = i;
		
		printf("Nome do programa: %s - índice: %d\n", programas[i].nome, programas[i].indice);
		
		i++;

		sleep(1);
	 }

	fclose(arquivo);

	return 0;
}
