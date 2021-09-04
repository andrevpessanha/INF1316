#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "sim.h"
int tempo, qtdPaginas;
void validaParametros (char * tipoAlg, int tamPagina, int tamMemFis) {

	// Valida tipo de algoritmo (NRU, 2CH ou LFU)
	if( strcmp(tipoAlg,"NRU") != 0 && strcmp(tipoAlg,"2CH") != 0 && strcmp(tipoAlg,"LFU") != 0) {
		printf("Tipo de algoritmo inválido!\n\nAlgoritmos suportados:\n");
		printf("Not-Recently-Used: NRU\nFIFO Segunda Chance: 2CH\nLeast-Frequently-Used: LFU\n\n");
		exit(1);
	}
	// Valida tamanho da página (entre 8 e 32KB)
	if(tamPagina < 8 || tamPagina > 32) {
		printf("Tamanho da página inválido!\n");
		exit(1);
	}
	// Valida tamanho da memória física (entre 1 e 16MB)
	if(tamMemFis < 1 || tamMemFis > 16) {
		printf("Tamanho da memória física inválido!\n");
		exit(1);
	}
}

int * criaVetPaginas(int qtdPaginas) {
	int i, *vp;
        
	vp = (int *) malloc(sizeof(int) * qtdPaginas);

	for(i = 0; i < qtdPaginas; i++) 
		vp[i] = -1;
	
	return vp;
}

TabelaPagina * criaVetTabelaPaginas(int tamPagina) {

	TabelaPagina *vtp;
	int i, qtdTabs;

	// Quantidade de tabelas de páginas = 2^(Bits Número Página)
	// Bits Número Página = 32 - Bits Deslocamento
	// Bits Deslocamento = log na base 2 do tamanho da página em Bytes
	// Exemplo: tamPagina = 8KB = 8000 Bytes; Bits Deslocamento = log(8000) = 13 bits

	qtdTabs = pow(2, 32 - (int)(ceil(log2(tamPagina*1000))));
	//printf("QtdTabs: %d\n", qtdTabs);
	
	vtp = (TabelaPagina*) malloc(sizeof(TabelaPagina) * qtdTabs);
	//printf("qtdTabs: %d\n", qtdTabs);

	// Inicializa tabelas de páginas
    for(i = 0; i < qtdTabs; i++) {
		vtp[i].R = 0;
		vtp[i].M = 0;            
		vtp[i].tempoUltimoAcesso = 0; 
		vtp[i].tempoUltimaCarga = -1;
		vtp[i].frequenciaAcesso = 0;
		vtp[i].indiceVP = -1; // Não está na memória física
    }
    return vtp;
}

int buscaPaginaNRU(TabelaPagina * vtp, int * vp, int qtdPaginas) {

	int i, R, M, indiceTP, flag, * nRnM , * nRM, * RnM, * RM, tam;
	int indicenRnM = 0, indicenRM = 0, indiceRnM = 0, indiceRM = 0;
	int menorTempo, posMenor; // tempo e posição da página mais antiga
	
	nRnM = criaVetPaginas(qtdPaginas); // vetor de páginas não referenciadas e não modificadas
	nRM = criaVetPaginas(qtdPaginas); // vetor de páginas não referenciadas e modificadas
	RnM = criaVetPaginas(qtdPaginas); // vetor de páginas referenciadas e não modificadas
	RM = criaVetPaginas(qtdPaginas); // vetor de páginas referenciadas e modificadas

	// percorre vetor de páginas
	for (i = 0; i < qtdPaginas; i++) {
		indiceTP = vp[i];
		if (vtp[indiceTP].R == 0) {
			if(vtp[indiceTP].M == 0) {
				//não referenciada e não modificada
				nRnM[indicenRnM] = i; // Armazena índice vp
				indicenRnM++;
			}
			else {
				//não referenciada e modificada
				nRM[indicenRM] = i;
				indicenRM++;
			}
		} 
		else {
			if(vtp[indiceTP].M == 0) {
				//referenciada e não modificada
				RnM[indiceRnM] = i;
				indiceRnM++;
			}
			else {
				//referenciada e modificada
				RM[indiceRM] = i;
				indiceRM++;
			}
		}
	}

	int * vet;
	if ( nRnM[0] != -1){
		vet = nRnM;
		tam = indicenRnM;
	} 
	else if (nRM[0] != -1) {
		vet = nRM;
		tam = indicenRM;
	} 
	else if (RnM[0] != -1) {
		vet = RnM;
		tam = indiceRnM;
	} 
	else {
		vet = RM;
		tam = indiceRM;
	}
	
	// vet contém todos os índices de vp de um dos critérios acima
	// caso exista mais de um elemento no vetor, tirar o mais antigo
	posMenor = 0;
	indiceTP = vp[vet[0]];
	menorTempo = vtp[indiceTP].tempoUltimoAcesso;
	
	for(i = 1; i < tam ; i++) {
		indiceTP = vp[vet[i]];
		if (menorTempo > vtp[indiceTP].tempoUltimoAcesso) {
			posMenor = vet[i];
			menorTempo = vtp[indiceTP].tempoUltimoAcesso;
		}
	}
	return posMenor;
}

int buscaPagina2CH(TabelaPagina * vtp, int * vp, int qtdPaginas) {
	int i, indiceTP, achou = 0, menorTempo, posMenor;
	int posMenorTodos, primeiraBusca = 1;
	
	if(vtp[vp[0]].R == 0) return 0;

	posMenor = 0;
	indiceTP = vp[0];
	menorTempo = vtp[indiceTP].tempoUltimaCarga;

	while(!achou) {
		
		for (i = 0; i < qtdPaginas; i++) {
			indiceTP = vp[i];
			if (menorTempo > vtp[indiceTP].tempoUltimaCarga) {
				posMenor = i;
				menorTempo = vtp[indiceTP].tempoUltimaCarga;
			}
		}
		if(primeiraBusca) {
			posMenorTodos = posMenor;
			primeiraBusca = 0;
		}
		
		if(posMenor != -1) {
			// Se Bit R ligado: Desliga e dá segunda chance (Busca próximo menor)
			if(vtp[vp[posMenor]].R == 1) {
				indiceTP = vp[posMenor];
				vtp[indiceTP].R = 0;  // Desliga bit
				menorTempo = vtp[indiceTP].tempoUltimaCarga;
				posMenor = -1; // Indica que não tem próximo menor (Fim da fila)
			}
			else { 
				achou = 1;
				break;
			}
		}	
		// Todos os Bits R = 1 foram desligados (Remove o primeiro da fila)
		else return posMenorTodos;
	}
	return posMenor;
}

int buscaPaginaLFU(TabelaPagina * vtp, int * vp, int qtdPaginas) {

	int i, indiceTP, posMenor, freqMenor;
	int flag = 0; // indica se página foi encontrada na primeira busca

	posMenor = 0;
	freqMenor = vtp[0].frequenciaAcesso;

	for(i = 1; i < qtdPaginas; i++) {

		indiceTP = vp[i];

		// menor número de referências e último acesso antigo
		if(vtp[indiceTP].frequenciaAcesso < freqMenor && vtp[indiceTP].tempoUltimoAcesso > tempo - qtdPaginas) {
			freqMenor = vtp[indiceTP].frequenciaAcesso;
			posMenor = i;
			flag = 1;
		} 
	}
	if(flag == 0) { // não encontrou página com requisitos especificados
		// LFU normal
		posMenor = 0;
		freqMenor = vtp[0].frequenciaAcesso;

		for(i = 1; i < qtdPaginas; i++) {

			indiceTP = vp[i];

			if(vtp[indiceTP].frequenciaAcesso < freqMenor) {
				freqMenor = vtp[indiceTP].frequenciaAcesso;
				posMenor = i;
				flag = 1;
			} 
		}
	}
	return posMenor;

}

void removePagina(TabelaPagina * vtp, int * vp, int indiceVP, int indiceTP) {

	int aux;

	aux = vp[indiceVP];
	
	vtp[aux].R = 0;
	vtp[aux].M = 0;
	vtp[aux].tempoUltimaCarga = -1;

	vtp[indiceTP].indiceVP = aux;
	vp[indiceVP] = indiceTP; 
}

void zeraBitReferencia(TabelaPagina * vtp, int *vp, int qtdPaginas) {
	int i, indiceTP;

	for(i = 0; i < qtdPaginas; i++) {
		indiceTP = vp[i];
		if(indiceTP != -1)
			vtp[indiceTP].R = 0;
	}
}

void executaSimuladorVirtual(FILE *arq, char * tipoAlg, int tamPagina, int tamMemFis) {

	unsigned int addr;
	char rw;
	int faltaPagina, posAtual = 0, indiceTP;
	int *vp; // Vetor de Páginas
	TabelaPagina *vtp; //Vetor de Tabela de Páginas
	int tempoZeraRefs = 0, qtdFaltas = 0, qtdEscritas = 0; // Contadores
	tempo = 0;
	//Quantidade de páginas = Tamanho memória Física (Em KB) / Tamanho da página
	qtdPaginas = (tamMemFis * 1000)/tamPagina;
	
	vp = criaVetPaginas(qtdPaginas);
	vtp = criaVetTabelaPaginas(tamPagina);

	while(fscanf(arq, "%x %c", &addr, &rw) == 2) {

		if((!strcmp(tipoAlg,"NRU") || !strcmp(tipoAlg,"2CH")) && tempoZeraRefs == qtdPaginas) {
			zeraBitReferencia(vtp, vp, qtdPaginas);
			tempoZeraRefs = 0;
		}

		//printf("\nTempo: %d\nEndereço: %x Acesso: %c\n", tempo, addr, rw);

		// Calcula o índice da tabela de páginas
		indiceTP = addr >> (int)(ceil(log2(tamPagina * 1000)));
		// Verifica se está na memória física
		faltaPagina = vtp[indiceTP].indiceVP;

		if(faltaPagina != -1) {
			//printf("Falta de Página? Não\n");
		}
		else {
			//printf("Falta de Página? Sim\n");
			qtdFaltas++;
			
			//Verifica se tem espaço livre no Vetor de Páginas
			if(posAtual < qtdPaginas) {
				//printf("Remover Página? Não\n");
				vp[posAtual] = indiceTP;
				vtp[indiceTP].indiceVP = posAtual;
				posAtual++;
			}
			else { // Vetor de Páginas cheio (Remove uma Página)
				//printf("Remover Página? Sim (Indice %d)\n", indiceVP);
				int indiceVP, aux;
				
				if(!strcmp(tipoAlg, "NRU")) indiceVP = buscaPaginaNRU(vtp, vp, qtdPaginas);
				else if(!strcmp(tipoAlg, "2CH")) indiceVP = buscaPagina2CH(vtp, vp, qtdPaginas);
				else if(!strcmp(tipoAlg, "LFU")) indiceVP = buscaPaginaLFU(vtp, vp, qtdPaginas);
				
				// Escreve modificações
				aux = vp[indiceVP];
				if(vtp[aux].M == 1) qtdEscritas++;

				removePagina(vtp, vp, indiceVP, indiceTP);
			}
		}

		//Atualiza dados da nova página
		vtp[indiceTP].R = 1;
		if(rw == 'W') vtp[indiceTP].M = 1;
		vtp[indiceTP].frequenciaAcesso++;
		vtp[indiceTP].tempoUltimoAcesso = tempo;
		if(vtp[indiceTP].tempoUltimaCarga == -1) vtp[indiceTP].tempoUltimaCarga = tempo;
      	
		tempoZeraRefs++;
		tempo++;
	}

	printf("Número de Faltas de Páginas: %d\n", qtdFaltas);
	printf("Número de Páginas Escritas: %d\n", qtdEscritas);

	free(vp);
	free(vtp);
}

int main(int argc, char *argv[]) {
	
	FILE *arq;
	char tipoAlg[4], caminho[50] = "Testes/";
	int tamPagina, tamMemFis;

	if(argc < 5) {
		printf("Quantidade inválida de parâmetros!\n");
		printf("Exemplo: sim-virtual LFU arquivo.log 8 16\n");
		exit(1);
	}

	strcpy(tipoAlg, argv[1]);
	tamPagina = atoi(argv[3]); 
	tamMemFis = atoi(argv[4]);
	strcat(caminho, argv[2]);
	arq = fopen(caminho,"r");

	if (!arq) {
    	printf("Erro ao abrir arquivo!\n");
    	exit(1);
	}
	
	validaParametros(tipoAlg, tamPagina, tamMemFis);

	printf("\nExecutando o simulador...\n");
	printf("Arquivo de entrada: %s\n", argv[2]);
	printf("Tamanho da memória física: %d MB\n", tamMemFis);
	printf("Tamanho das páginas: %d KB\n", tamPagina);
	printf("Algoritmo de substituição: %s\n", tipoAlg);
    	
	executaSimuladorVirtual(arq, tipoAlg, tamPagina, tamMemFis);

	fclose(arq);
	
	return 0;

}
