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

#define END 0x10000 // Endereço da memória compartilhada com o interpretador
#define TEMPO_ESPERA 3 // Tempo limite no estado de espera para operações de I/O
#define SEGUNDOS 60 // Período de execução 
#define TEMPO_MAX 3 // Período de teste do escalonamento em minutos

Fila *filaEspera, *filaPronto;
Processo *processoCorrente = NULL;
int cpuOciosa; // 0: Não está; 1: Está ociosa
int tempoCpuOciosa;

/*Estrutura com os dados do programa (enviados pelo interpretador)*/
struct infoPrograma {
    char nome[20]; /* Nome do programa */ 
    int indice;  /* índice do programa equivalente à ordem que foi lida */
    int inicio; /* início da execução */
    char inicioP[20]; /*início da execução com base em outro programa*/
    int duracao; /*tempo de duração*/
}; 
typedef struct infoPrograma InfoPrograma;

void criaFilas() {
	filaEspera = fila_cria();
	filaPronto = fila_cria();
}

void recebeuSinalIO(int signal);

char * converteParaChar(int pidPai) {
	int pid[4];
	char * novo = (char*) malloc (5 * sizeof(char));
	int i;

	i = 3;
	while(pidPai) {
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
        	if(execv(path, args) < 0) { // Tenta executar o programa
    			printf("Erro ao executar %s.\n", nomeProg); 
    		}
	} 
	else { // Pai (Pausa o programa que está executando no processo filho)
		kill(pid, SIGSTOP);
		sleep(1);
	    printf("\nEscalonador: Criou processo filho do %s com pid %d\n", nomeProg, pid);
		printf("\nEscalonador: %s recebeu sinal SIGSTOP\n", nomeProg);

		if(signal(SIGUSR1,recebeuSinalIO) == SIG_ERR) {
			printf("Processo pai recebeu sinal I/O (Chamando rotina)\n");
		}
	}
	return pid;
}

void recebeuSinalIO(int signal) {
	if(signal == SIGUSR1) { 
		processoCorrente->enviouSinalIO = 1;
	}
}

/*Adiciona processo que recebeu I/O à fila de espera*/
void trataSinalIO() {
	printf("%s enviou sinal SIGUSR1 (I/O)\n", processoCorrente->nome);
	processoCorrente->status = ESPERA;
	fila_insere(filaEspera, copiaProcesso(processoCorrente));
	kill(processoCorrente->pid, SIGSTOP);
	printf("%s recebeu SIGSTOP e entrou na fila de Espera\n", processoCorrente->nome);
	exibeFila(filaEspera, "Espera");
}
	
/*Incrementa 1 u.t em cada processo em espera*/
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

/*Mata todos os processos depois de um tempo máximo de execução*/
void mataProcessos() {
	Processo * processoCorrente;
	
	processoCorrente = filaPronto->ini;

	while (processoCorrente != NULL)
	{
		kill(processoCorrente->pid,SIGKILL);
		printf("\nProcesso %s recebeu SIGKILL e foi finalizado\n", processoCorrente->nome);
		removeProcesso(filaPronto, processoCorrente);
		liberaProcesso(processoCorrente);
		exibeFila(filaPronto, "Pronto");
		processoCorrente = processoCorrente->prox;
	}
}

void escalonadorRealTime() {
	int tempo = 0;
	int minutos = 0;
	int qtdProcessos = filaPronto->qtd;	
	
	// Informações sobre o processo corrente
	char * nomePCorrente;
	int iniPCorrente, duracaoPCorrente, pidCorrente, fimPCorrente;
	
	printf("\n\n***** Iniciando Escalonamento (Real Time) *****\n\n");

	exibeFila(filaPronto, "Pronto");
	
	while (qtdProcessos > 0) {
		
		if (processoCorrente ==  NULL) {

			processoCorrente = filaPronto->ini;

			while(processoCorrente != NULL) {

				iniPCorrente = processoCorrente->inicio;
				duracaoPCorrente = processoCorrente->duracao;
				pidCorrente = processoCorrente->pid;
				fimPCorrente = iniPCorrente + duracaoPCorrente;
				nomePCorrente = processoCorrente->nome;

				if (tempo == iniPCorrente) {
					processoCorrente->status = PROCESSANDO;
					removeProcesso(filaPronto, processoCorrente);
					kill(pidCorrente, SIGCONT);
					printf("\n%s recebeu SIGCONT\n", nomePCorrente);
					exibeFila(filaPronto, "Pronto");
					break;
				} 
				else {
					processoCorrente = processoCorrente->prox;
				}
			}
		}
		else {

			if(processoCorrente->enviouSinalIO) {
				processoCorrente->enviouSinalIO  = 0;
				trataSinalIO();
			}
			if (tempo == fimPCorrente) {
				if (processoCorrente->status == ESPERA) {
					processoCorrente = NULL;
				} 
				else {
					kill(pidCorrente,SIGSTOP);
					printf("\n%s recebeu SIGSTOP\n", nomePCorrente);
					processoCorrente->status = PRONTO;
					fila_insere(filaPronto, copiaProcesso(processoCorrente));
					exibeFila(filaPronto, "Pronto");
					processoCorrente = NULL;
				}
			
			}
		}
		
		sleep(1);
		
		printf("Tempo = %d\n", tempo + 1);
		
		tempo++;
		if (tempo == 59) minutos++;
		tempo = tempo % SEGUNDOS;
		
		
		if (minutos == TEMPO_MAX) { 
			printf("\nTempo máximo atingido!\n");
			mataProcessos();
			break;
		} 
		
		if(filaEspera->qtd > 0) atualizaTempoEmEspera();
	}
	printf("\n\n***** Fim do Escalonamento (Real Time) *****\n\n");
}

int main(void)
{
	int segmento;
	InfoPrograma *dados;
	int i = 0;
	Processo * novo;

	criaFilas();
	
	dados = (InfoPrograma *) shmat (END, 0, 0);

	while (strlen(dados[i].nome) != 0) {
		// processo com dependência causal
		if(dados[i].inicio == -1) { 
			Processo * aux = buscaProcesso(filaPronto,dados[i].inicioP);
			if (aux == NULL) {
				printf("\nNão foi possível criar o processo do %s - %s não existe\n", dados[i].nome,dados[i].inicioP);
				i++;
				continue;
			}
			dados[i].inicio = aux->inicio + aux->duracao + 1; // inícia depois do outro acabar
		}
		// período de execução do processo não vai além do início do próximo minuto
		if((dados[i].inicio + dados[i].duracao) <= 60) {
			
			int intervalo_permitido = verificaIntervaloDeExecucao(dados[i].inicio,dados[i].duracao,filaPronto);
			
			// intervalo válido - não tem outros processos em execução no seu intervalo
			if (intervalo_permitido == 1) {
				
				int pid = criaNovoProcesso(dados[i].nome, dados[i].indice);
				printf("\nNome do programa: %s - início: %d  - duração: %d - índice: %d\n", dados[i].nome, dados[i].inicio, dados[i].duracao, dados[i].indice);
		
				//Insere processo na fila de prontos
				novo = criaProcesso(dados[i].nome, pid, dados[i].inicio,dados[i].duracao); 
				fila_insere(filaPronto, novo);

			} 
			else {
				printf("\nNão foi possível criar o processo do %s - intervalo de tempo já estava ocupado\n", dados[i].nome);
			}

		} 
		else {
			printf("\nNão foi possível criar o processo do %s - período de execução além de um minuto\n", dados[i].nome);
		}

		i++;
	}

	escalonadorRealTime();

	// libera a memória compartilhada do processo
	shmdt(dados);

	// libera a memória compartilhada
	shmctl(segmento, IPC_RMID, 0);

	return 0;
}
