#include <stdlib.h>
#include <stdio.h>
#include "fila.h"

Fila* fila_cria() {
    Fila* f = (Fila*)malloc(sizeof(Fila));
    f->ini=f->fim = NULL;
    f->qtd = 0;
    return f;
}

Processo * criaProcesso(char * nome, int pid, int prioridade) {
	Processo* p = (Processo*)malloc(sizeof(Processo));
    p->nome = nome;
    p->prioridade = prioridade;
    p->pid = pid;
	p->status = PRONTO;
    p->tempoEmExecucao = 0;
    p->tempoEmEspera = 0;
	p->enviouSinalIO = 0;
    p->prox=NULL;

	return p;
}

Processo * copiaProcesso(Processo *p) {
	Processo *novo;
	novo = criaProcesso(p->nome, p->pid, p->prioridade);
	novo->tempoEmExecucao = p->tempoEmExecucao;
	novo->tempoEmEspera = p->tempoEmEspera;
	novo->enviouSinalIO = p->enviouSinalIO;
	return novo;
}
 
void fila_insere(Fila* f, Processo *p) {
    p->prox = NULL;
    if (f->fim != NULL)
        f->fim->prox = p;
    else
        f->ini=p;
    f->fim = p;
    f->qtd++;
}

/*Busca processo com maior prioridade e retorna uma cópia do mesmo*/
Processo * buscaProcessoMaiorPrioridade(Fila *f) {
	Processo *pMaior, *aux;

	if(f->qtd == 0){
		return NULL;
	}
	pMaior = f->ini;
	aux = pMaior->prox;
	while(aux != NULL) {
		//Quanto menor, maior será a prioridade
		if(aux->prioridade < pMaior->prioridade)
			pMaior = aux;
		aux = aux->prox;
	}
	return copiaProcesso(pMaior);
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

	if(f->qtd == 0) {
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
			if(aux->pid == f->fim->pid) { 
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
	if(f->qtd == 0) {
		printf("   Fila vazia!\n\n");
		return;
	}
	aux = f->ini;
	while(aux!=NULL) {
		printf("  Nome: %s - Prioridade: %d\n",aux->nome, aux->prioridade);
		aux = aux->prox;
	}
	printf("\n");
	return;
}

void liberaProcesso(Processo *p) {
	free(p);
}

void fila_libera (Fila* f) {
    Processo* q = f->ini;
    while(q!=NULL){
    	Processo* t = q->prox;
    	free(q);
    	q = t;
    }
    free(f);
}

int fila_vazia (Fila * f) {
    return (f->ini == NULL);
}
