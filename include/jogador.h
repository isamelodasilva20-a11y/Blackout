#ifndef JOGADOR_H
#define JOGADOR_H

#include "jogo.h"

void movimentar_jogador(Jogador *jogador, Mapa *mapa, Direcao direcao);

// Usadas na visao 3D: girar 90 graus no lugar e andar uma casa para tras
// sem mudar para onde o jogador esta olhando
void girar_jogador(Jogador *jogador, bool paraDireita);

void recuar_jogador(Jogador *jogador, Mapa *mapa);

// origem = canto superior esquerdo do mapa na tela, em pixels
void desenharJogador2D(Jogo *jogo, Vector2 origem, int tamanhoCasa);

// Seta que mostra a posicao e a direcao do jogador (usada no minimapa)
void desenharSetaJogador2D(Jogador *jogador, Vector2 origem, int tamanhoCasa);

#endif