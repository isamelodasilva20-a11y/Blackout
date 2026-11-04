#ifndef ROBO_INTERCEPTADOR_H
#define ROBO_INTERCEPTADOR_H

#include "jogo.h"

// Ponteiro de função para calcular a posição alvo do robô
typedef Posicao (*ComportamentoInterceptador)(Jogo *jogo, Robo *robo);

typedef struct {
    Robo base;
    ComportamentoInterceptador comportamento;
} RoboInterceptador;

void inicializarRoboInterceptador(RoboInterceptador *robo, Posicao posicaoInicial);
void atualizarRoboInterceptador(Jogo *jogo, RoboInterceptador *robo);

#endif