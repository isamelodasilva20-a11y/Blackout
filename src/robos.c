#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib.h"
#include "jogo.h"
#include "robos.h"

// Converte o Vector2 de direção no índice da imagem (0: Frente, 1: Costas, 2: Lado)
int obterIndiceDirecaoRobo(Vector2 direcao) {
    if (direcao.y < 0) return 1;  // Cima (Costas)
    if (direcao.x != 0) return 2; // Esquerda / Direita (Lado)
    return 0;                     // Baixo (Frente)
}

void robosAdicionar(Robo **cabeca, Posicao posicao, TipoRobo tipo, int andar) {
    (void)andar;

    Robo *novo = (Robo*)malloc(sizeof(Robo));
    if (!novo) return;

    novo->posicao = posicao;
    novo->posicaoReal.x = (float)posicao.coluna;
    novo->posicaoReal.y = (float)posicao.linha;
    novo->tipo = tipo;
    novo->direcao = (Vector2){ 0.0f, 1.0f };
    
    novo->next = *cabeca;
    *cabeca = novo;
}

void robosLiberar(Robo **cabeca) {
    if (cabeca == NULL) return;
    
    Robo *atual = *cabeca;
    while (atual != NULL) {
        Robo *temp = atual;
        atual = atual->next;
        free(temp);
    }
    *cabeca = NULL;
}

void robosAtualizarContinuo(Jogo *jogo, float deltaTime) {
    if (jogo == NULL) return;

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        if (atual->estado != ESTADO_DESLIGADO) {
            float passo = atual->velocidade * deltaTime;
            Posicao alvo;

            if (atual->tipo == ROBO_INTERCEPTADOR) {
                alvo = jogo->jogador.posicao;
                switch (jogo->jogador.direcao) {
                    case DIR_CIMA:     alvo.linha -= 2; break;
                    case DIR_BAIXO:    alvo.linha += 2; break;
                    case DIR_ESQUERDA: alvo.coluna -= 2; break;
                    case DIR_DIREITA:  alvo.coluna += 2; break;
                }
            } else if (atual->tipo == ROBO_PERSEGUIDOR) {
                alvo = jogo->jogador.posicao;
            } else if (atual->tipo == ROBO_TEMPORIZADOR) {
                alvo = jogo->professor.posicao;
            }

            float dx = (float)alvo.coluna - atual->posicaoReal.x;
            float dy = (float)alvo.linha - atual->posicaoReal.y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist > 0.05f) {
                atual->direcao = (Vector2){ dx / dist, dy / dist };
                atual->posicaoReal.x += atual->direcao.x * passo;
                atual->posicaoReal.y += atual->direcao.y * passo;
            }

            atual->posicao.coluna = (int)(atual->posicaoReal.x + 0.5f);
            atual->posicao.linha  = (int)(atual->posicaoReal.y + 0.5f);
        }
        atual = atual->next;
    }
}

// Assinatura usando (Jogo *jogo, Vector2 origem, int tamanhoTile) para bater com robos.h
void desenharRobos(Jogo *jogo, Vector2 origem, int tamanhoTile) {
    if (!jogo) return;

    Robo *r = jogo->robos;
    while (r != NULL) {
        int idxTex = obterIndiceDirecaoRobo(r->direcao);
        float escadaX = (r->direcao.x > 0) ? -1.0f : 1.0f;

        Texture2D tex;
        if (r->tipo == ROBO_TEMPORIZADOR) {
            tex = jogo->texRoboTemporizador[idxTex];
        } else if (r->tipo == ROBO_PERSEGUIDOR) {
            tex = jogo->texRoboPerseguidor[idxTex];
        } else {
            tex = jogo->texRoboInterceptador[idxTex];
        }

        if (tex.id > 0) {
            Vector2 posTela = {
                origem.x + r->posicaoReal.x * (float)tamanhoTile,
                origem.y + r->posicaoReal.y * (float)tamanhoTile
            };

            Rectangle srcRec = { 0.0f, 0.0f, (float)tex.width * escadaX, (float)tex.height };
            Rectangle destRec = { posTela.x, posTela.y, (float)tamanhoTile, (float)tamanhoTile };
            Vector2 origin = { 0.0f, 0.0f };

            DrawTexturePro(tex, srcRec, destRec, origin, 0.0f, WHITE);
        }

        r = r->next;
    }
}

void checarVitoriaDerrotaRoboRoxo(Jogo *jogo) {
    if (jogo == NULL) return;

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        if (atual->tipo == ROBO_TEMPORIZADOR) {
            if (atual->posicao.linha == jogo->professor.posicao.linha &&
                atual->posicao.coluna == jogo->professor.posicao.coluna) {
                jogo->telaAtual = TELA_NORMAL;
            }
        }
        atual = atual->next;
    }
}