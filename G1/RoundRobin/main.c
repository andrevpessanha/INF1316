#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include "fila.h"

#define END 0x5  // Endereço da memória compartilhada com o interpretador
#define TEMPO_KILL 10 // Intervalo de tempo para kill cada processo
#define TEMPO_ESPERA 3 // Tempo limite no estado de espera

Fila *filaEspera, *filaPronto;
Processo *processoCorrente = NULL;
int cpuOciosa; // 0: Não 1: Sim
int tempoCpuOciosa;

/*Estrutura com os dados do programa (enviados pelo interpretador)*/
struct infoPrograma {
    char nome[20]; /* Nome do programa */ 
    int indice;  /* índice do programa equivalente à ordem que foi lida */
}; 
typedef struct infoPrograma InfoPrograma ;

void recebeuSinalIO(int signal);
void atualizaEiniciaProcessoCorrente();

char * converteParaChar(int pidPai){
	int pid[4];
	char * novo = (char*) malloc (5 * sizeof(char));
	int i;

	i = 3;
	while(pidPai){
		pid[i] = pidPai % 10;
		pidPai /= 10;
		i--;
	}
	for(i = 0; i < 4; i++){
		novo[i] = pid[i] + '0';
	}
	novo[i] = '\0';

	return novo;
}

/* Função que cria um novo processo para cada programa lido e retorna o pid */
int criaNovoProcesso (char *nomeProg, int index) {
	char path[] = "../Programas/";
	int pid, pidPai;
	char * pidPaiChar;

	pidPai = getpid();
	pid = fork();
	pidPaiChar = converteParaChar(pidPai);

	if(pid < 0) { // Erro
		printf("Erro ao criar processo filho.\n");
    		exit(-1);
    	}
	else if (pid == 0) { // Filho 
		char * args[2];
    		args[0] = pidPaiChar;
    		args[1] = NULL;
		strcat(path, nomeProg);  // Programas/programa1 (Exemplo)
        	if(execv(path, args) < 0) // Tenta executar o programa
    			printf("Erro ao executar %s.\n", nomeProg); 
	} 
	else { // Pai (Pausa o programa que está executando no processo filho)
		kill(pid, SIGSTOP);
		sleep(1);
	    printf("Escalonador: Criou processo filho %s com pid %d\n", nomeProg, pid);
		printf("Escalonador: %s recebeu sinal SIGSTOP\n", nomeProg);
		if(signal(SIGUSR1,recebeuSinalIO) == SIG_ERR){
			printf("Processo pai recebeu sinal I/O (Chamando rotina)\n");
		}
	}
	return pid;
}

void recebeuSinalIO(int signal){
	if(signal == SIGUSR1) { 
		processoCorrente->enviouSinalIO = 1;
	}
}

/*Adiciona processo que recebeu I/O à fila de espera*/
void trataSinalIO(){
	printf("%s enviou sinal SIGUSR1 (I/O)\n", processoCorrente->nome);
	processoCorrente->status = ESPERA;
	fila_insere(filaEspera, copiaProcesso(processoCorrente));
	kill(processoCorrente->pid, SIGSTOP);
	printf("%s recebeu SIGSTOP e entrou na fila de Espera\n", processoCorrente->nome);
	exibeFila(filaEspera, "Espera");
	if(filaPronto->qtd > 0) atualizaEiniciaProcessoCorrente();
	else{
		processoCorrente = NULL;
		printf("\nCPU está ociosa!\n");
		cpuOciosa = 1;
		tempoCpuOciosa = 0;
	}
}

/*Incrementa 1 u.t. em cada processo em espera*/
void atualizaTempoEmEspera() {
	Processo *aux;
	aux = filaEspera->ini;

	while(aux != NULL) {
		aux->tempoEmEspera++;
		if(aux->tempoEmEspera == TEMPO_ESPERA) { // Terminou espera I/O
			aux->status = PRONTO;
			aux->tempoEmEspera = 0;
			removeProcesso(filaEspera, aux);
			printf("%s terminou I/O - Removido da Fila de Espera\n\n", aux->nome);
			fila_insere(filaPronto, copiaProcesso(aux));
			exibeFila(filaPronto, "Pronto");
		}
		aux = aux->prox;
	}
}

/*Retira processo da fila de prontos e inicia sua execução*/
void atualizaEiniciaProcessoCorrente(){
	processoCorrente = fila_retira(filaPronto);
	if(processoCorrente == NULL) {
		printf("Fila Pronto vazia e chamou fila_retira\n");
		 return ;
	}
	kill(processoCorrente->pid, SIGCONT);
	processoCorrente->status = PROCESSANDO;
	printf("%s recebeu SIGCONT\n", processoCorrente->nome);
	if(cpuOciosa) { // Terminou I/O
		tempoCpuOciosa = 0;
		cpuOciosa = 0;
	}
	exibeFila(filaPronto, "Pronto");
}

void executaEscalonamentoRoundRobin() {
	
	int i, status, qtdProcessos, tempoKill = 0;

	printf("\n\n***** Iniciando Escalonamento (Round Robin) *****\n\n");

	exibeFila(filaPronto, "Pronto");

	qtdProcessos = filaPronto->qtd;
	atualizaEiniciaProcessoCorrente();

	// Cada iteração passa 1 u.t.
	while(qtdProcessos > 0){ 

		sleep(1); // Time-Slice de 1 u.t.
		
		if(cpuOciosa) { // Se a CPU estiver ociosa
			tempoCpuOciosa++;
			printf("CPU ociosa por %d u.t.\n", tempoCpuOciosa);
			if(filaEspera->qtd > 0) atualizaTempoEmEspera();
		}
		else { // Tem algum processo em execução
			processoCorrente->tempoEmExecucao++;
			tempoKill++;
			printf("%s - Tempo de processamento: %d u.t.\n", processoCorrente->nome, processoCorrente->tempoEmExecucao);
			if(filaEspera->qtd > 0) atualizaTempoEmEspera();
			if(processoCorrente->enviouSinalIO) {
				processoCorrente->enviouSinalIO  = 0;
				trataSinalIO();
			}
		}
	
		// Atualiza processo corrente com próximo processo da Fila Pronto
		if(filaPronto->qtd > 0) {
			if(processoCorrente != NULL && processoCorrente->status == PROCESSANDO) {
				kill(processoCorrente->pid, SIGSTOP);
				printf("\n%s recebeu SIGSTOP\n\n", processoCorrente->nome);
				processoCorrente->status = PRONTO;
				fila_insere(filaPronto, copiaProcesso(processoCorrente));
			}
			atualizaEiniciaProcessoCorrente();
		}
		
		if(tempoKill >= 10) { // Kill no processo corrente
			if(processoCorrente != NULL) {
				kill(processoCorrente->pid, SIGKILL);
				qtdProcessos--;
				printf("\n%s recebeu SIGKILL - Restam %d processo(s)\n\n", processoCorrente->nome, qtdProcessos);
				liberaProcesso(processoCorrente);
				tempoKill = 0;
				if(qtdProcessos == 0) break;
				if(filaPronto->qtd > 0) atualizaEiniciaProcessoCorrente();
				else cpuOciosa = 0;
			}
		}
		
	}
	printf("\n\n***** Fim do Escalonamento (Round Robin) *****\n\n");
}

int main(void) {
	int segmento,segmento2;
	InfoPrograma *dados;
	int i = 0, j;
	Processo * novo;
	
	filaEspera = fila_cria();
	filaPronto = fila_cria();

	dados = (InfoPrograma *) shmat (END, 0, 0);

	while (strlen(dados[i].nome) != 0) {
		int pid = criaNovoProcesso(dados[i].nome, dados[i].indice);
		printf("Nome do programa: %s - pid: %d\n\n",dados[i].nome, pid);
		novo = criaProcesso(dados[i].nome, pid); 
		fila_insere(filaPronto, novo);
		i++;
	}

	executaEscalonamentoRoundRobin();

	// libera a memória compartilhada do processo
	shmdt(dados);

	// libera a memória compartilhada
	shmctl(segmento, IPC_RMID, 0);

	return 0;
}
