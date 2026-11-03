#ifndef HUD_H
#define HUD_H

#include "jogo.h"

/*
 * HUD (barra de informacoes) e adaptacao ao tamanho da janela.
 *
 * O jogo foi desenhado pensando numa janela de 1024x640 (tamanho "base").
 * Quando a janela muda de tamanho, tudo e multiplicado por uma escala:
 * 1.0 no tamanho base, 2.0 numa janela com o dobro do tamanho, etc.
 */

#define LARGURA_BASE    1024
#define ALTURA_BASE     640
#define HUD_ALTURA_BASE 60   // altura da barra do HUD no tamanho base, em pixels

// Escala atual da janela em relacao ao tamanho base (usa o lado que "cabe" menos)
float hudEscala(void);

// Altura da barra do HUD na janela atual, em pixels
int hudAlturaBarra(void);

// Calcula o tamanho de cada casa e onde o mapa 2D comeca (origem) para ele
// ocupar o maximo da tela acima do HUD, centralizado
void hudLayoutMapa(Mapa *mapa, Vector2 *origem, int *tamanhoCasa);

// Barra do HUD: vidas, andar, baterias, pendrive, cronometro, objetivo e controles
void desenharHUD(Jogo *jogo);

// Tela escura de vitoria, com o tempo total
void desenharTelaVitoria(Jogo *jogo);

#endif
