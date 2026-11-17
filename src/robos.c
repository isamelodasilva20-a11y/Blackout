#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "raylib.h"
#include "estrutura.h"
#include "robos.h"
#include "mapa.h"

Robo *roboQueBateu = NULL;
int passosRecuo = 0;

// Converte o Vector2 de direcao no indice da imagem (0: Frente, 1: Costas, 2: Lado)
int obterIndiceDirecaoRobo(Vector2 direcao) {
    if (direcao.y < 0) return 1;  // Cima (Costas)
    if (direcao.x != 0) return 2; // Esquerda / Direita (Lado)
    return 0;                     // Baixo (Frente)
}

void robosAdicionar(Robo **cabeca, Posicao posicao, TipoRobo tipo, int andar) {
    Robo *novo = (Robo *)malloc(sizeof(Robo));
    if (novo == NULL) return;

    novo->posicao = posicao;
    novo->posicaoInicial = posicao;
    novo->posicaoAnterior = posicao;
    novo->posicaoReal = (Vector2){ (float)posicao.coluna, (float)posicao.linha };
    novo->tipo = tipo;
    novo->estado = ESTADO_NORMAL;
    novo->velocidade = 2.0f + (andar - 1) * 0.2f;
    novo->direcao = (Vector2){ 0, 0 };
    novo->tempoParalisado = 0.0f;
    novo->next = *cabeca;
    *cabeca = novo;
}

void robosLiberar(Robo **cabeca) {
    roboQueBateu = NULL;
    passosRecuo = 0;
    
    if (cabeca == NULL) return;

    Robo *atual = *cabeca;
    while (atual != NULL) {
        Robo *temp = atual;
        atual = atual->next;
        free(temp);
    }
    *cabeca = NULL;
}

// Verifica se a casa e caminhavel respeitando paredes (#), mesas (M) e computadores (C)
static bool casaLivreParaRobo(Mapa *mapa, int linha, int coluna, Posicao destinoPermitido) {
    if (!mapa || !mapa->matriz) return false;
    if (linha < 0 || linha >= mapa->linhas || coluna < 0 || coluna >= mapa->colunas) {
        return false;
    }
    if (linha == destinoPermitido.linha && coluna == destinoPermitido.coluna) {
        return true;
    }
    return !mapaCasaBloqueada(mapa, linha, coluna);
}

// Busca em Largura (BFS): calcula o proximo passo do caminho mais curto
static Posicao calcularProximoPassoBFS(Mapa *mapa, Posicao origem, Posicao destino) {
    if (!mapa || !mapa->matriz) return origem;
    if (origem.linha == destino.linha && origem.coluna == destino.coluna) return origem;

    int linhas = mapa->linhas;
    int colunas = mapa->colunas;
    if (linhas <= 0 || colunas <= 0 || linhas > 256 || colunas > 256) return origem;

    static bool visitado[256][256];
    static Posicao pai[256][256];
    static Posicao fila[256 * 256];

    for (int i = 0; i < linhas; i++) {
        for (int j = 0; j < colunas; j++) {
            visitado[i][j] = false;
        }
    }

    int inicio = 0, fim = 0;
    visitado[origem.linha][origem.coluna] = true;
    pai[origem.linha][origem.coluna] = origem;
    fila[fim++] = origem;

    int dL[4] = { -1, 1, 0, 0 };
    int dC[4] = { 0, 0, -1, 1 };
    bool encontrou = false;

    while (inicio < fim) {
        Posicao atual = fila[inicio++];

        if (atual.linha == destino.linha && atual.coluna == destino.coluna) {
            encontrou = true;
            break;
        }

        for (int i = 0; i < 4; i++) {
            int nL = atual.linha + dL[i];
            int nC = atual.coluna + dC[i];

            if (nL >= 0 && nL < linhas && nC >= 0 && nC < colunas && !visitado[nL][nC]) {
                if (casaLivreParaRobo(mapa, nL, nC, destino)) {
                    visitado[nL][nC] = true;
                    pai[nL][nC] = atual;
                    fila[fim++] = (Posicao){ nL, nC };
                }
            }
        }
    }

    if (!encontrou) return origem;

    Posicao passo = destino;
    while (!(pai[passo.linha][passo.coluna].linha == origem.linha &&
             pai[passo.linha][passo.coluna].coluna == origem.coluna)) {
        passo = pai[passo.linha][passo.coluna];
    }

    return passo;
}

// Escolhe a direcao que mais afasta o robo do jogador
static Posicao calcularPassoFuga(Mapa *mapa, Robo *robo, Posicao posJogador) {
    Posicao dirs[4] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };
    Posicao melhor = robo->posicao;
    float maiorDist = -1.0f;

    for (int i = 0; i < 4; i++) {
        int nL = robo->posicao.linha + dirs[i].linha;
        int nC = robo->posicao.coluna + dirs[i].coluna;

        if (casaLivreParaRobo(mapa, nL, nC, robo->posicao)) {
            if (nL == robo->posicaoAnterior.linha && nC == robo->posicaoAnterior.coluna) {
                continue;
            }
            float dL = (float)(posJogador.linha - nL);
            float dC = (float)(posJogador.coluna - nC);
            float dist = dL * dL + dC * dC;

            if (dist > maiorDist) {
                maiorDist = dist;
                melhor = (Posicao){ nL, nC };
            }
        }
    }

    // Se entrou em um beco ou sala sem saida, permite dar meia-volta pela casa anterior
    if (melhor.linha == robo->posicao.linha && melhor.coluna == robo->posicao.coluna) {
        if (casaLivreParaRobo(mapa, robo->posicaoAnterior.linha, robo->posicaoAnterior.coluna, robo->posicao)) {
            melhor = robo->posicaoAnterior;
        }
    }

    return melhor;
}

void robosAtualizarContinuo(Jogo *jogo, float deltaTime) {
    if (jogo == NULL) return;

    bool lanternaAtiva = (jogo->jogador.tempoLanterna > 0.0f && jogo->jogador.bateria > 0);

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        if (atual->estado == ESTADO_DESLIGADO) {
            atual = atual->next;
            continue;
        }

        // Se o robô estiver paralisado pela lanterna, apenas desconta o tempo e não anda
        if (atual->tempoParalisado > 0.0f) {
            atual->tempoParalisado -= deltaTime;
            if (atual->tempoParalisado < 0.0f) atual->tempoParalisado = 0.0f;
            atual = atual->next;
            continue;
        }

        Posicao alvo = jogo->jogador.posicao;

        if (atual->tipo == ROBO_INTERCEPTADOR) {
            // Calcula distância até o jogador: se já estiver perto (<= 2 casas), vai direto nele!
            int distL = abs(atual->posicao.linha - jogo->jogador.posicao.linha);
            int distC = abs(atual->posicao.coluna - jogo->jogador.posicao.coluna);

            if (distL + distC <= 2) {
                alvo = jogo->jogador.posicao;
            } else {
                alvo = jogo->jogador.posicao;
                switch (jogo->jogador.direcao) {
                    case DIR_CIMA:     alvo.linha -= 2; break;
                    case DIR_BAIXO:    alvo.linha += 2; break;
                    case DIR_ESQUERDA: alvo.coluna -= 2; break;
                    case DIR_DIREITA:  alvo.coluna += 2; break;
                }
            }
        } else if (atual->tipo == ROBO_TEMPORIZADOR) {
            if (jogo->professor.posicao.linha >= 0 && jogo->professor.posicao.coluna >= 0) {
                alvo = jogo->professor.posicao;
            } else {
                alvo = jogo->jogador.posicao;
            }
        }

        // Laranja e Vermelho recuam se a lanterna tiver bateria, e perseguem se estiver sem
        if (atual->tipo == ROBO_PERSEGUIDOR || atual->tipo == ROBO_INTERCEPTADOR) {
            atual->estado = lanternaAtiva ? ESTADO_FUGINDO : ESTADO_NORMAL;
        }

        float distDestinoX = (float)atual->posicao.coluna - atual->posicaoReal.x;
        float distDestinoY = (float)atual->posicao.linha - atual->posicaoReal.y;
        float distRestante = sqrtf(distDestinoX * distDestinoX + distDestinoY * distDestinoY);

        if (distRestante < 0.05f) {
            atual->posicaoReal.x = (float)atual->posicao.coluna;
            atual->posicaoReal.y = (float)atual->posicao.linha;

            if (atual == roboQueBateu && passosRecuo > 0) {
                passosRecuo--;
                if (passosRecuo == 0) {
                    roboQueBateu = NULL;
                    jogo->jogador.tempoInvulneravel = 0.0f;
                }
            }

            Posicao proximaCasa = atual->posicao;

            if (atual->estado == ESTADO_FUGINDO || (atual == roboQueBateu && passosRecuo > 0)) {
                proximaCasa = calcularPassoFuga(&jogo->mapa, atual, jogo->jogador.posicao);
            } else {
                Posicao alvo = jogo->jogador.posicao;

                if (atual->tipo == ROBO_TEMPORIZADOR) {
                    if (jogo->professor.posicao.linha >= 0 && jogo->professor.posicao.coluna >= 0) {
                        alvo = jogo->professor.posicao;
                    }
                } else if (atual->tipo == ROBO_INTERCEPTADOR) {
                    Posicao alvoIntercept = jogo->jogador.posicao;
                    switch (jogo->jogador.direcao) {
                        case DIR_CIMA:     alvoIntercept.linha -= 2; break;
                        case DIR_BAIXO:    alvoIntercept.linha += 2; break;
                        case DIR_ESQUERDA: alvoIntercept.coluna -= 2; break;
                        case DIR_DIREITA:  alvoIntercept.coluna += 2; break;
                    }
                    if (casaLivreParaRobo(&jogo->mapa, alvoIntercept.linha, alvoIntercept.coluna, alvoIntercept)) {
                        alvo = alvoIntercept;
                    }
                }

                proximaCasa = calcularProximoPassoBFS(&jogo->mapa, atual->posicao, alvo);

                // Fallback caso o ponto de interceptacao esteja inacessivel
                if (proximaCasa.linha == atual->posicao.linha &&
                    proximaCasa.coluna == atual->posicao.coluna &&
                    (alvo.linha != jogo->jogador.posicao.linha || alvo.coluna != jogo->jogador.posicao.coluna) &&
                    atual->tipo != ROBO_TEMPORIZADOR) {
                    proximaCasa = calcularProximoPassoBFS(&jogo->mapa, atual->posicao, jogo->jogador.posicao);
                }
            }

            atual->posicaoAnterior = atual->posicao;
            atual->posicao = proximaCasa;
        }

        // Movimento suave continuo em direcao a proxima casa valida
        float dx = (float)atual->posicao.coluna - atual->posicaoReal.x;
        float dy = (float)atual->posicao.linha - atual->posicaoReal.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 0.001f) {
            float vel = atual->velocidade;
            if (vel <= 0.0f) {
                vel = 2.0f;
            }

            if (atual->estado == ESTADO_FUGINDO) {
                vel = vel * 0.85f;
            } else if (atual == roboQueBateu && passosRecuo > 0) {
                vel = vel * 1.35f;
            }

            float passo = vel * deltaTime;
            if (passo > dist) 
                passo = dist;

            atual->direcao = (Vector2){ dx / dist, dy / dist };
            atual->posicaoReal.x += atual->direcao.x * passo;
            atual->posicaoReal.y += atual->direcao.y * passo;
        }

        atual = atual->next;
    }
}

void desenharRobos(Jogo *jogo, Vector2 origem, int tamanhoTile) {
    if (!jogo) return;

    Robo *r = jogo->robos;
    while (r != NULL) {
        if (r->estado == ESTADO_DESLIGADO) {
            r = r->next;
            continue;
        }

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

            // Fica azulado enquanto recua por causa da bateria da lanterna
            Color tint = (r->estado == ESTADO_FUGINDO) ? SKYBLUE : WHITE;
            DrawTexturePro(tex, srcRec, destRec, origin, 0.0f, tint);
        }

        r = r->next;
    }
}

void checarVitoriaDerrotaRobos(Jogo *jogo) {
    if (jogo == NULL) return;

    // Atualiza o cronômetro de invulnerabilidade do jogador
    if (jogo->jogador.tempoInvulneravel > 0.0f) {
        jogo->jogador.tempoInvulneravel -= GetFrameTime();
        if (jogo->jogador.tempoInvulneravel < 0.0f) {
            jogo->jogador.tempoInvulneravel = 0.0f;
        }
    }

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        // 1) ROBÔ ROXO (Temporizador):
        if (atual->tipo == ROBO_TEMPORIZADOR) {
            // 1a. Checa se o JOGADOR tocou no robô roxo para paralisá-lo usando a lanterna
            if (jogo->jogador.tempoInvulneravel <= 0.0f && jogo->jogador.bateria > 0 && atual->tempoParalisado <= 0.0f) {
                float dxJog = atual->posicaoReal.x - (float)jogo->jogador.posicao.coluna;
                float dyJog = atual->posicaoReal.y - (float)jogo->jogador.posicao.linha;
                float distJog = sqrtf(dxJog * dxJog + dyJog * dyJog);

                int mesmaCasaJog = (atual->posicao.linha == jogo->jogador.posicao.linha &&
                                    atual->posicao.coluna == jogo->jogador.posicao.coluna);

                if (distJog < 0.8f || mesmaCasaJog) {
                    jogo->jogador.bateria = 0;            // Consome a carga da lanterna
                    jogo->jogador.tempoInvulneravel = 1.5f;
                    atual->tempoParalisado = 5.0f;        // Robô roxo fica parado por 5 segundos!
                }
            }

            // 1b. Checa se o robô roxo (não paralisado) encostou no PROFESSOR
            if (!jogo->professor.ajudado && !jogo->jogador.temPendrive) {
                float dx = atual->posicaoReal.x - (float)jogo->professor.posicao.coluna;
                float dy = atual->posicaoReal.y - (float)jogo->professor.posicao.linha;
                float dist = sqrtf(dx * dx + dy * dy);

                int mesmaCasa = (atual->posicao.linha == jogo->professor.posicao.linha &&
                                 atual->posicao.coluna == jogo->professor.posicao.coluna);

                if (dist < 0.8f || mesmaCasa) {
                    jogo->telaAtual = TELA_GAME_OVER;
                    return;
                }
            }
        }
        
        // 2) ROBÔS LARANJA E VERMELHO: Checa colisão com o JOGADOR
        else if (atual->tipo == ROBO_PERSEGUIDOR || atual->tipo == ROBO_INTERCEPTADOR) {
            if (jogo->jogador.tempoInvulneravel <= 0.0f) {
                float dx = atual->posicaoReal.x - (float)jogo->jogador.posicao.coluna;
                float dy = atual->posicaoReal.y - (float)jogo->jogador.posicao.linha;
                float dist = sqrtf(dx * dx + dy * dy);

                int mesmaCasa = (atual->posicao.linha == jogo->jogador.posicao.linha &&
                                 atual->posicao.coluna == jogo->jogador.posicao.coluna);

                if (dist < 0.8f || mesmaCasa) {
                    jogo->jogador.tempoInvulneravel = 2.0f;

                    if (jogo->jogador.bateria > 0) {
                        // Consome toda a carga da bateria (1 pilha = 1 proteção)
                        jogo->jogador.bateria = 0;
                    } else {
                        // Sem bateria: perde 1 coração
                        jogo->jogador.vidas--;
                        if (jogo->jogador.vidas <= 0) {
                            jogo->jogador.vidas = 0;
                            jogo->telaAtual = TELA_GAME_OVER;
                            return;
                        }
                    }
                }
            }
        }

        atual = atual->next;
    }
}
