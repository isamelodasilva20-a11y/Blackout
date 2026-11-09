#ifndef ROBOS_H
#define ROBOS_H

#include "raylib.h"
#include "estrutura.h"

// Protótipos das funções de robôs
void robosAdicionar(Robo **cabeca, Posicao posicao, TipoRobo tipo, int andar);
void robosAtualizarContinuo(Jogo *jogo, float deltaTime);
int obterIndiceDirecaoRobo(Vector2 direcao);
void desenharRobos(Jogo *jogo, Vector2 origem, int tamanhoTile);
void checarVitoriaDerrotaRobos(Jogo *jogo);
void robosLiberar(Robo **cabeca);

#endif