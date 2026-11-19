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

void desenharTelaVitoria(struct Jogo *jogo, EntradaRanking ranking[], int totalRanking);

// Escurece o jogo e mostra "PAUSADO" por cima (o main chama enquanto a pausa durar)
void desenharTelaPausa(void);

// Tela inicial com a historia do jogo (telainicial.c). So volta quando o
// jogador aperta ENTER ou fecha a janela.
void telaInicialMostrar(char *nomeJogador, int maxChars);

// Game over animado com o resumo da partida (andar, tempo e pendrives do jogo)
void desenharTelaGameOver(struct Jogo *jogo);

// Libera a textura que o game over carrega (robo cinza). Chamar ao fechar o jogo.
void hudDescarregar(void);

#endif