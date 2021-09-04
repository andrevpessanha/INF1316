#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct tabelaPagina {
    int R; //Referenciada (0: Não 1: Sim)
    int M; //Modificada   (0: Não 1: Sim)
    int tempoUltimoAcesso;
    int tempoUltimaCarga;
    int frequenciaAcesso;
    int indiceVP;  // Indice do Vetor de Páginas ou -1 (Não está na memória) 
} TabelaPagina;

int * criaVetPaginas(int qtdPages);

TabelaPagina * criaVetTabelaPaginas(int tamPage);

void executaSimuladorVirtual(FILE *arq, char * tipoAlg, int tamPage, int tamMemFis);
