#include <stdlib.h>
#include <stdio.h>

enum status {
	PRONTO, PROCESSANDO, ESPERA
};

typedef enum status Status;

struct processo {
    char * nome; // Nome do Programa
    int pid;
    Status status; // Estado atual do processo
    int tempoEmExecucao;
    int tempoEmEspera;
    int enviouSinalIO; // 0: Não 1: Sim
    struct processo * prox;
};

typedef struct processo Processo;

struct fila {
    Processo* ini;
    Processo* fim;
    int qtd;  // Quantidade de Processos
};

typedef struct fila Fila;
 
Fila* fila_cria ();

Processo * criaProcesso(char * nome, int pid);

Processo * copiaProcesso(Processo *p);
 
void fila_insere(Fila* f, Processo *p);

Processo * fila_retira (Fila* f);

void removeProcesso(Fila *f, Processo *p);

void exibeFila(Fila *f, char *nomeFila);

void liberaProcesso(Processo *p);
 
void fila_libera (Fila* f);

int fila_vazia (Fila * f);
