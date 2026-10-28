#ifndef MAPA_H
#define MAPA_H

#include "jogo.h"

int  mapaCarregar(Jogo *jogo, const char *arquivo);
void mapaLiberar(Mapa *mapa);

#endif
