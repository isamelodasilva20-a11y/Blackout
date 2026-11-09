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
    Robo *novo = (Robo*)malloc(sizeof(Robo));
    if (novo == NULL) return;

    novo->posicao = posicao;
    novo->posicaoReal = (Vector2){ (float)posicao.coluna, (float)posicao.linha }; // <--- VITAL
    novo->tipo = tipo;
    novo->velocidade = 2.0f;
    novo->direcao = (Vector2){ 0, 0 };
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
static int casaEhCaminhoValido(Mapa *mapa, int linha, int coluna) {
    if (!mapa || !mapa->matriz) return 0;
    if (linha < 0 || linha >= mapa->linhas || coluna < 0 || coluna >= mapa->colunas) {
        return 0;
    }
    char c = mapa->matriz[linha][coluna];
    return (c != '#'); // Retorna 1 se NÃO for parede
}

void robosAtualizarContinuo(Jogo *jogo, float deltaTime) {
    if (jogo == NULL) return;

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        Posicao alvo = jogo->jogador.posicao;

        if (atual->tipo == ROBO_INTERCEPTADOR) {
            alvo = jogo->jogador.posicao;
            switch (jogo->jogador.direcao) {
                case DIR_CIMA:     alvo.linha -= 2; break;
                case DIR_BAIXO:    alvo.linha += 2; break;
                case DIR_ESQUERDA: alvo.coluna -= 2; break;
                case DIR_DIREITA:  alvo.coluna += 2; break;
            }
        } else if (atual->tipo == ROBO_TEMPORIZADOR) {
            // Se a posição do professor ainda não foi definida, usa o jogador como fallback
            if (jogo->professor.posicao.linha >= 0 && jogo->professor.posicao.coluna >= 0) {
                alvo = jogo->professor.posicao;
            } else {
                alvo = jogo->jogador.posicao;
            }
        }

        // Se a posição do alvo for parede ou fora do mapa, fallback para o jogador
        if (!casaEhCaminhoValido(&jogo->mapa, alvo.linha, alvo.coluna)) {
            alvo = jogo->jogador.posicao;
        }

        // Verifica se chegou ao centro do ladrilho atual para escolher o próximo destino
        float distDestinoX = (float)atual->posicao.coluna - atual->posicaoReal.x;
        float distDestinoY = (float)atual->posicao.linha - atual->posicaoReal.y;
        float distRestante = sqrtf(distDestinoX * distDestinoX + distDestinoY * distDestinoY);

        if (distRestante < 0.05f) {
            atual->posicaoReal.x = (float)atual->posicao.coluna;
            atual->posicaoReal.y = (float)atual->posicao.linha;

            Posicao dirs[4] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} }; // Cima, Baixo, Esquerda, Direita
            Posicao melhorProxima = atual->posicao;
            float menorDistancia = 999999.0f;

            for (int i = 0; i < 4; i++) {
                int proxLinha = atual->posicao.linha + dirs[i].linha;
                int proxColuna = atual->posicao.coluna + dirs[i].coluna;

                // Não deixa o robô voltar exatamente para trás de onde acabou de vir (evita oscilação/vibração)
                int veioDeLinha = (int)(atual->posicaoReal.x - atual->direcao.x + 0.5f);
                int veioDeColuna = (int)(atual->posicaoReal.y - atual->direcao.y + 0.5f);

                if (casaEhCaminhoValido(&jogo->mapa, proxLinha, proxColuna)) {
                    float dLinha = (float)(alvo.linha - proxLinha);
                    float dColuna = (float)(alvo.coluna - proxColuna);
                    float distAteAlvo = dLinha * dLinha + dColuna * dColuna; // Distância ao quadrado

                    // Penaliza levemente voltar para a casa anterior para evitar travamento em 180°
                    if (proxLinha == veioDeLinha && proxColuna == veioDeColuna) {
                        distAteAlvo += 2.0f;
                    }

                    if (distAteAlvo < menorDistancia) {
                        menorDistancia = distAteAlvo;
                        melhorProxima.linha = proxLinha;
                        melhorProxima.coluna = proxColuna;
                    }
                }
            }

            atual->posicao = melhorProxima;
        }

        // Movimento suave continuo em direção à próxima casa válida
        float dx = (float)atual->posicao.coluna - atual->posicaoReal.x;
        float dy = (float)atual->posicao.linha - atual->posicaoReal.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 0.001f) {
            float vel = (atual->velocidade > 0.0f) ? atual->velocidade : 2.0f;
            float passo = vel * deltaTime;
            if (passo > dist) passo = dist;

            atual->direcao = (Vector2){ dx / dist, dy / dist };
            atual->posicaoReal.x += atual->direcao.x * passo;
            atual->posicaoReal.y += atual->direcao.y * passo;
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

void checarVitoriaDerrotaRobos(Jogo *jogo) {
    if (jogo == NULL) return;

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        // 1) ROBO ROXO: Game Over se tocar no PROFESSOR
        if (atual->tipo == ROBO_TEMPORIZADOR) {
            float dx = atual->posicaoReal.x - (float)jogo->professor.posicao.coluna;
            float dy = atual->posicaoReal.y - (float)jogo->professor.posicao.linha;
            float dist = sqrtf(dx * dx + dy * dy);

            // Checa colisão por proximidade contínua OU pela mesma casa da grelha
            int mesmaCasa = (atual->posicao.linha == jogo->professor.posicao.linha &&
                             atual->posicao.coluna == jogo->professor.posicao.coluna);

            if (dist < 0.8f || mesmaCasa) {
                jogo->telaAtual = TELA_GAME_OVER; 
            }
        }
        
        // 2) ROBOS LARANJA E VERMELHO: Game Over se tocarem no JOGADOR
        else if (atual->tipo == ROBO_PERSEGUIDOR || atual->tipo == ROBO_INTERCEPTADOR) {
            float dx = atual->posicaoReal.x - (float)jogo->jogador.posicao.coluna;
            float dy = atual->posicaoReal.y - (float)jogo->jogador.posicao.linha;
            float dist = sqrtf(dx * dx + dy * dy);

            int mesmaCasa = (atual->posicao.linha == jogo->jogador.posicao.linha &&
                             atual->posicao.coluna == jogo->jogador.posicao.coluna);

            if (dist < 0.8f || mesmaCasa) {
                jogo->telaAtual = TELA_GAME_OVER; 
            }
        }

        atual = atual->next;
    }
}