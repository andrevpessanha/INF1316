#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fila.h"

Fila* fila_cria () {
    Fila* f = (Fila*)malloc(sizeof(Fila));
    f->ini=f->fim = NULL;
    f->qtd = 0;
    return f;
}

/* Verifica se o intervalo é permitido - retorna 1 se estiver livre, 0 caso contrário*/
int verificaIntervaloDeExecucao(int inicio, int duracao, Fila * f) {

	Processo *pAtual;
	if (f->qtd == 0) {
		return 1;
	}
	pAtual = f->ini;

	int iniPAtual, duracaoPAtual, fimPAtual, fim;
	fim = inicio + duracao;

	while ( pAtual != NULL) {

		
		iniPAtual = pAtual->inicio; // início processo atual
		duracaoPAtual = pAtual->duracao; // duração do processo atual
		fimPAtual = iniPAtual + duracaoPAtual; // tempo final do processo atual
		
		// Processo enviado tem execução com interseção com o processo atual
		if ((inicio >= iniPAtual && inicio <= fimPAtual) || (inicio <= iniPAtual && fim >= iniPAtual)) {
			
			return 0;
		} 
		
		pAtual = pAtual->prox;
		
	}

	return 1;
}

Processo * criaProcesso(char * nome, int pid, int inicio, int duracao) {
	Processo* p = (Processo*)malloc(sizeof(Processo));
    	p->nome = nome;
    	p->pid = pid;
    	p->inicio = inicio;
    	p->duracao = duracao;
		p->status = PRONTO;
		p->enviouSinalIO = 0;
    	p->tempoEmExecucao = 0;
    	p->tempoEmEspera = 0;
    	p->prox=NULL;

	return p;
}

Processo * copiaProcesso(Processo *p) {
	Processo *novo;
	novo = criaProcesso(p->nome, p->pid, p->inicio, p->duracao);
	novo->tempoEmExecucao = p->tempoEmExecucao;
	novo->tempoEmEspera = p->tempoEmEspera;
	return novo;
}
 
void fila_insere (Fila* f, Processo *p) {
    p->prox = NULL;
    if (f->fim != NULL)
        f->fim->prox = p;
    else
        f->ini=p;
    f->fim = p;
    f->qtd++;
}

Processo * fila_retira (Fila* f) {
    Processo* t;
    if(f->ini == NULL)
        return NULL;
    t = f->ini;
    f->ini = t->prox;
    if (f->ini == NULL)
        f->fim = NULL;
    f->qtd--;
    return t;
}

void removeProcesso(Fila *f, Processo *p) {
	Processo *aux, *ant;

	if(f->qtd == 0){
		printf("Fila vazia!\n");
		return;
	}
	if(f->ini->pid == p->pid) {
		f->ini = f->ini->prox;
		(f->qtd)--;
		if(f->qtd == 0) f->ini = f->fim = NULL;
		return;		
	}
	aux = f->ini->prox;
	ant = f->ini;
	while(aux != NULL) {
		if(aux->pid == p->pid) {
			ant->prox = aux->prox;
			if(aux->pid == f->fim->pid){
				f->fim = ant;
			}
			(f->qtd)--;
			if(f->qtd == 0) f->ini = f->fim = NULL;
			return;
		}
		ant = aux;
		aux = aux->prox;

	}	
	printf("Não encontrou o processo %s na fila!\n", p->nome);
	return;
}

void exibeFila(Fila *f, char *nomeFila) {
	Processo *aux;
	
	printf("\nProcessos na Fila de %s:\n", nomeFila);
	if(f == NULL) {
		printf("Fila não foi criada!\n");
		return;
	}
	if(f->qtd == 0){
		printf("Fila vazia!\n\n");
		return;
	}
	aux = f->ini;
	while(aux != NULL) {
		printf("Nome: %s - Início: %d - Duração: %d\n",aux->nome, aux->inicio, aux->duracao);
		aux = aux->prox;
	}
	printf("\n");
	return;
}
/*Busca na fila de processos, o processo com o nome enviado*/
Processo * buscaProcesso(Fila *f, char *nomePrograma) {
	Processo *aux;
	aux = f->ini;
	while (aux != NULL) {
		if (strcmp(aux->nome,nomePrograma) == 0) {
			return aux;
		}
		aux = aux->prox;
	}
	return NULL;
}

void liberaProcesso(Processo *p) {
	free(p);
}

void fila_libera (Fila* f) {
    Processo* q = f->ini;
    while(q!=NULL) {
    	Processo* t = q->prox;
    	free(q);
    	q = t;
    }
    free(f);
}

int fila_vazia (Fila * f) {
    return (f->ini == NULL);
}
