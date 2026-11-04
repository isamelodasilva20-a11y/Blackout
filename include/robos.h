#ifndef ROBOS_H
#define ROBOS_H

#include "jogo.h"

void robosAdicionar(Robo **cabeca, Posicao posicao, TipoRobo tipo);
void robosAtualizar(Jogo *jogo);
void checarColisoes(Jogo *jogo);
void robosLiberar(Robo **cabeca);

#endif