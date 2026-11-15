#ifndef HUD_H
#define HUD_H

#include "raylib.h"

// Avisa o compilador que estas structs existem, SEM criar um novo typedef
struct Jogo;
struct Mapa;

#define LARGURA_BASE     1024
#define ALTURA_BASE      640
#define HUD_ALTURA_BASE  60  

float hudEscala(void);
int hudAlturaBarra(void);

// Usamos "struct Mapa" e "struct Jogo" aqui para evitar qualquer conflito
void hudLayoutMapa(struct Mapa *mapa, Vector2 *origem, int *tamanhoCasa);
void desenharHUD(struct Jogo *jogo);
void desenharTelaVitoria(struct Jogo *jogo);

// Escurece o jogo e mostra "PAUSADO" por cima (o main chama enquanto a pausa durar)
void desenharTelaPausa(void);

// Tela inicial com a historia do jogo (telainicial.c). So volta quando o
// jogador aperta ENTER/ESPACO ou fecha a janela.
void telaInicialMostrar(void);

#endif