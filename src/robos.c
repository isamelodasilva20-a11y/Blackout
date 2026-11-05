#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib.h"
#include "jogo.h"
#include "robos.h"

void robosAdicionar(Robo **cabeca, Posicao posicao, TipoRobo tipo, int andar) {
    Robo *novo = (Robo *)malloc(sizeof(Robo));
    if (novo == NULL) return;

    novo->posicao = posicao;
    novo->posicaoInicial = posicao;
    novo->tipo = tipo;
    novo->estado = ESTADO_NORMAL;
    novo->posicaoReal = (Vector2){ (float)posicao.coluna, (float)posicao.linha };
    novo->direcao = (Vector2){ 0.0f, 0.0f };

    // Dificuldade progressiva pelos 3 andares (1.0x, 1.25x, 1.50x)
    float multiplicadorAndar = 1.0f + ((andar - 1) * 0.25f);

    if (tipo == ROBO_TEMPORIZADOR) {
        novo->velocidade = 1.2f * multiplicadorAndar;
    } else {
        novo->velocidade = 2.5f * multiplicadorAndar;
    }

    novo->next = *cabeca;
    *cabeca = novo;
}

void robosAtualizarContinuo(Jogo *jogo, float deltaTime) {
    if (jogo == NULL) return;

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        if (atual->estado != ESTADO_DESLIGADO) {
            float passo = atual->velocidade * deltaTime;
            Posicao alvo;

            if (atual->tipo == ROBO_INTERCEPTADOR) {
                // LARANJA: Antecipa 2 casas à frente da direção do jogador
                alvo = jogo->jogador.posicao;
                switch (jogo->jogador.direcao) {
                    case DIR_CIMA:     alvo.linha -= 2; break;
                    case DIR_BAIXO:    alvo.linha += 2; break;
                    case DIR_ESQUERDA: alvo.coluna -= 2; break;
                    case DIR_DIREITA:  alvo.coluna += 2; break;
                }
            } else if (atual->tipo == ROBO_PERSEGUIDOR) {
                // VERMELHO: Persegue a posição atual do jogador
                alvo = jogo->jogador.posicao;
            } else if (atual->tipo == ROBO_TEMPORIZADOR) {
                // ROXO: Anda em direção à posição exata do PROFESSOR
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

            // Atualiza a casa da grelha lógica para colisões
            atual->posicao.coluna = (int)(atual->posicaoReal.x + 0.5f);
            atual->posicao.linha  = (int)(atual->posicaoReal.y + 0.5f);
        }
        atual = atual->next;
    }
}

int obterIndiceDirecaoRobo(Vector2 direcao) {
    if (fabsf(direcao.x) > fabsf(direcao.y)) {
        return (direcao.x > 0) ? 3 : 2; // 3: Direita, 2: Esquerda
    } else {
        return (direcao.y > 0) ? 1 : 0; // 1: Baixo (Frente), 0: Cima (Costas)
    }
}

void desenharRobos(Jogo *jogo, Vector2 origem, int tamanhoTile) {
    if (jogo == NULL) return;

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        if (atual->estado != ESTADO_DESLIGADO) {
            float posX = origem.x + (atual->posicaoReal.x * tamanhoTile);
            float posY = origem.y + (atual->posicaoReal.y * tamanhoTile);

            int idxDir = obterIndiceDirecaoRobo(atual->direcao);

            Texture2D texturaAtual;
            if (atual->tipo == ROBO_INTERCEPTADOR) {
                texturaAtual = jogo->texRoboInterceptador[idxDir];
            } else if (atual->tipo == ROBO_PERSEGUIDOR) {
                texturaAtual = jogo->texRoboPerseguidor[idxDir];
            } else {
                texturaAtual = jogo->texRoboTemporizador[idxDir]; // Corrigido
            }

            Color corFiltro = (atual->estado == ESTADO_FUGINDO) ? BLUE : WHITE;

            Rectangle origemTextura = { 0, 0, (float)texturaAtual.width, (float)texturaAtual.height };
            Rectangle destino = { posX, posY, (float)tamanhoTile, (float)tamanhoTile };

            DrawTexturePro(texturaAtual, origemTextura, destino, (Vector2){0, 0}, 0.0f, corFiltro);
        }
        atual = atual->next;
    }
}

void checarVitoriaDerrotaRoboRoxo(Jogo *jogo) {
    if (jogo == NULL) return;

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        if (atual->tipo == ROBO_TEMPORIZADOR && atual->estado != ESTADO_DESLIGADO) {
            if (atual->posicao.linha == jogo->professor.posicao.linha &&
                atual->posicao.coluna == jogo->professor.posicao.coluna) {
                jogo->telaAtual = TELA_NORMAL;
            }
        }
        atual = atual->next;
    }
}

void robosLiberar(Robo **cabeca) {
    Robo *atual = *cabeca;
    while (atual != NULL) {
        Robo *temp = atual;
        atual = atual->next;
        free(temp);
    }
    *cabeca = NULL;
}