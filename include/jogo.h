#ifndef JOGO_H
#define JOGO_H

#include "estrutura.h"

void jogoCarregar(Jogo *jogo);

void jogoCarregarMapaRobos(Jogo *jogo, char mapaTexto[100][100], int linhas, int colunas);

void jogoTick(Jogo *jogo);

void jogoDescarregar(Jogo *jogo);

#endif