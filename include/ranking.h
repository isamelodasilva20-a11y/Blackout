#ifndef RANKING_H
#define RANKING_H

#define MAX_RANKING 10
#define ARQUIVO_RANKING "ranking.txt"

#include "estrutura.h"

void rankingSalvar(const char *nome, float tempo);

int rankingCarregar(EntradaRanking lista[], int maxItens);

#endif