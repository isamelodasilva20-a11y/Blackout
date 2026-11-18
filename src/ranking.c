#include <stdio.h>
#include <string.h>
#include "estrutura.h"
#include "ranking.h"

// Ordena do menor tempo para o maior (Bubble Sort simples)
static void ordenarRanking(EntradaRanking lista[], int qtd) {
    for (int i = 0; i < qtd - 1; i++) {
        for (int j = 0; j < qtd - 1 - i; j++) {
            if (lista[j].tempo > lista[j + 1].tempo) {
                EntradaRanking temp = lista[j];
                lista[j] = lista[j + 1];
                lista[j + 1] = temp;
            }
        }
    }
}

int rankingCarregar(EntradaRanking lista[], int maxItens) {
    FILE *arquivo = fopen(ARQUIVO_RANKING, "r");
    if (arquivo == NULL) return 0; // Arquivo ainda não existe na primeira vez

    int qtd = 0;
    // Lê no formato: Nome;Tempo (ex: Joao Silva;45.32)
    while (qtd < maxItens && fscanf(arquivo, " %31[^;];%f\n", lista[qtd].nome, &lista[qtd].tempo) == 2) {
        qtd++;
    }

    fclose(arquivo);
    ordenarRanking(lista, qtd);
    return qtd;
}

void rankingSalvar(const char *nome, float tempo) {
    EntradaRanking lista[MAX_RANKING + 1];
    int qtd = rankingCarregar(lista, MAX_RANKING);

    // Garante um nome padrão caso o jogador aperte ENTER sem digitar nada
    if (nome == NULL || strlen(nome) == 0) {
        strcpy(lista[qtd].nome, "Anonimo");
    } else {
        strncpy(lista[qtd].nome, nome, 31);
        lista[qtd].nome[31] = '\0';
    }
    lista[qtd].tempo = tempo;
    qtd++;

    // Ordena todos (antigos + novo) pelo menor tempo
    ordenarRanking(lista, qtd);

    // Mantém apenas o Top 10
    if (qtd > MAX_RANKING) {
        qtd = MAX_RANKING;
    }

    // Reescreve o arquivo atualizado
    FILE *arquivo = fopen(ARQUIVO_RANKING, "w");
    if (arquivo == NULL) return;

    for (int i = 0; i < qtd; i++) {
        fprintf(arquivo, "%s;%.2f\n", lista[i].nome, lista[i].tempo);
    }

    fclose(arquivo);
}
