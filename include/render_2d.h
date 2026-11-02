#ifndef RENDER2D_H
#define RENDER2D_H

#include "jogo.h"

void desenharMapa2D(Mapa *mapa, int tamanhoCasa);
void desenharJogador2D(Jogo *jogo, int tamanhoCasa);
// liberada = true quando todas as baterias ja foram coletadas (escada verde)
void desenharSaida2D(Mapa *mapa, int tamanhoCasa, bool liberada);

#endif