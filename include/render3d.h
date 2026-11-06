#ifndef RENDER3D_H
#define RENDER3D_H

#include "estrutura.h"

/*
 * Visao 3D em primeira pessoa (jogo.modoVisao == 1).
 *
 * So muda o desenho: o jogador continua andando em grade, uma casa por tecla.
 * Cada casa do mapa vira um cubo de 1x1 no mundo 3D:
 *   x = coluna, z = linha, y = altura (0 = chao, 1 = teto)
 */

// Coloca a camera direto na casa e na direcao do jogador, sem animacao.
// Chamada quando o jogador liga o modo 3D.
void render3dReiniciarCamera(Jogo *jogo);

// Move a camera suavemente (cerca de 0,15 s) ate a casa/direcao atual do jogador.
// dt = tempo do ultimo quadro, em segundos.
void render3dAtualizarCamera(Jogo *jogo, float dt);

// Desenha o andar em 3D (chamar entre BeginDrawing e EndDrawing).
void desenharCena3D(Jogo *jogo);

// Minimapa 2D no canto superior direito da tela.
void desenharMinimapa(Jogo *jogo);

void desenharNickProfessor3D(Professor *professor);

#endif
