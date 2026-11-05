#ifndef JOGADOR_H
#define JOGADOR_H

#include "raylib.h"
#include "jogo.h"

// Movimentação e rotação
void movimentar_jogador(Jogador *jogador, Mapa *mapa, Direcao direcao);
void girar_jogador(Jogador *jogador, bool paraDireita);
void recuar_jogador(Jogador *jogador, Mapa *mapa);

// Funções de desenho
void desenharJogador2D(Jogo *jogo, Vector2 origem, int tamanhoCasa);
void desenharSetaJogador2D(Jogador *jogador, Vector2 origem, int tamanhoCasa);

#endif