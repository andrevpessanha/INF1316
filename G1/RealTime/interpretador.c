#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

/*Estrutura com os dados do programa (enviados pelo interpretador)*/
struct infoPrograma {
    char nome[20]; /* Nome do programa */ 
    int indice;  /* índice do programa equivalente à ordem que foi lida */
    int inicio; /* início da execução */
	char inicioP[20]; /*início da execução com base em outro programa*/
    int duracao; /*tempo de duração do programa*/
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
	InfoPrograma *programas;
	int i = 0, j;
	int dependencia = 0; //flag que sinaliza a dependência causal entre processos

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
		//Remove caracteres extras
		programa = strremove(programa,"Run <");
		programa = strremove(programa,"=<");
		programa = strremove(programa,">");
		programa = strremove(programa,"I");
		programa = strremove(programa,"D");

		// separa string por " " 
		char *ptr = strtok(programa," ");
		
		//antes do " ": nome do programa
		strcpy(programas[i].nome,ptr); 
		
		ptr = strtok(NULL, " ");
		
		// depois do " ": inicio
		for (j = 0; j < i; j++) {
			// se o momento de início é outro programa já definido
			if(strcmp(ptr,programas[j].nome) == 0) { 
				strcpy(programas[i].inicioP, ptr);
				dependencia = 1;
				break;
			}
		}
		if (dependencia == 1) {
			programas[i].inicio = -1;
			dependencia = 0;
		}
		else {
			programas[i].inicio = atoi(ptr); 
			strcpy(programas[i].inicioP,"");
		}
		
		ptr = strtok(NULL, " ");
		
		// depois do " ": duração
		programas[i].duracao = atoi(ptr); 

		programas[i].indice = i;
		
		printf("Nome do programa: %s - início: %d  - duração: %d - índice: %d\n", programas[i].nome, programas[i].inicio, programas[i].duracao, programas[i].indice);

		i++;

		sleep(1);
	 }

	fclose(arquivo);

	return 0;
}
