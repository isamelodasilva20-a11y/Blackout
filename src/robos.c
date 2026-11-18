#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "raylib.h"
#include "estrutura.h"
#include "robos.h"
#include "mapa.h"
#include "memoria.h"

Robo *roboQueBateu = NULL;
int passosRecuo = 0;

int obterIndiceDirecaoRobo(Vector2 direcao) {
    if (direcao.y < 0) return 1;
    if (direcao.x != 0) return 2;
    return 0;
}

void robosAdicionar(Robo **cabeca, Posicao posicao, TipoRobo tipo, int andar) {
    Robo *novo = memAlocar(sizeof(Robo));
    if (novo == NULL) return;

    novo->posicao = posicao;
    novo->posicaoInicial = posicao;
    novo->posicaoAnterior = posicao;
    novo->posicaoReal = (Vector2){ (float)posicao.coluna, (float)posicao.linha };
    novo->tipo = tipo;
    novo->estado = ESTADO_NORMAL;
    novo->direcao = (Vector2){ 0, 0 };
    novo->tempoParalisado = 0.0f;

    if (tipo == ROBO_TEMPORIZADOR) {
        novo->velocidade = 1.4f + (andar - 1) * 0.15f;
    } else if (tipo == ROBO_INTERCEPTADOR) {
        novo->velocidade = 4.8f + (andar - 1) * 0.3f;
    } else if (tipo == ROBO_PERSEGUIDOR) {
        novo->velocidade = 4.5f + (andar - 1) * 0.3f;
    } else {
        novo->velocidade = 4.4f + (andar - 1) * 0.25f;
    }

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
        memLiberar(temp);
    }
    *cabeca = NULL;
}

bool casaLivreParaRobo(Mapa *mapa, int linha, int coluna, Posicao destinoPermitido) {
    if (mapa == NULL || mapa->matriz == NULL) return false;
    if (linha < 0 || linha >= mapa->linhas || coluna < 0 || coluna >= mapa->colunas) return false;
    if (linha == destinoPermitido.linha && coluna == destinoPermitido.coluna) return true;
    
    return !mapaCasaBloqueada(mapa, linha, coluna);
}

Posicao calcularProximoPassoBFS(Mapa *mapa, Posicao origem, Posicao destino) {
    if (mapa == NULL || mapa->matriz == NULL) return origem;
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
    while (pai[passo.linha][passo.coluna].linha != origem.linha ||
           pai[passo.linha][passo.coluna].coluna != origem.coluna) {
        passo = pai[passo.linha][passo.coluna];
    }

    return passo;
}

Posicao calcularPassoFuga(Mapa *mapa, Robo *robo, Posicao posJogador) {
    int dL[4] = { -1, 1, 0, 0 };
    int dC[4] = { 0, 0, -1, 1 };
    Posicao melhor = robo->posicao;
    float maiorDist = -1.0f;

    for (int i = 0; i < 4; i++) {
        int nL = robo->posicao.linha + dL[i];
        int nC = robo->posicao.coluna + dC[i];

        if (casaLivreParaRobo(mapa, nL, nC, robo->posicao)) {
            if (nL == robo->posicaoAnterior.linha && nC == robo->posicaoAnterior.coluna) {
                continue;
            }
            float dx = (float)(posJogador.linha - nL);
            float dy = (float)(posJogador.coluna - nC);
            float dist = dx * dx + dy * dy;

            if (dist > maiorDist) {
                maiorDist = dist;
                melhor = (Posicao){ nL, nC };
            }
        }
    }

    if (melhor.linha == robo->posicao.linha && melhor.coluna == robo->posicao.coluna) {
        if (casaLivreParaRobo(mapa, robo->posicaoAnterior.linha, robo->posicaoAnterior.coluna, robo->posicao)) {
            melhor = robo->posicaoAnterior;
        }
    }

    return melhor;
}

bool buscarBateriaMaisProxima(Item *itens, Posicao origem, Posicao *destinoOut) {
    Item *it = itens;
    int menorDist = 999999;
    bool achou = false;

    while (it != NULL) {
        int dist = abs(origem.linha - it->posicao.linha) + abs(origem.coluna - it->posicao.coluna);
        if (dist < menorDist) {
            menorDist = dist;
            *destinoOut = it->posicao;
            achou = true;
        }
        it = it->next;
    }
    return achou;
}

void destruirBateriaNaPosicao(Item **itens, Posicao pos) {
    if (itens == NULL || *itens == NULL) return;

    Item *atual = *itens;
    Item *anterior = NULL;

    while (atual != NULL) {
        if (atual->posicao.linha == pos.linha && atual->posicao.coluna == pos.coluna) {
            if (anterior == NULL) {
                *itens = atual->next;
            } else {
                anterior->next = atual->next;
            }
            memLiberar(atual);
            return;
        }
        anterior = atual;
        atual = atual->next;
    }
}

bool temOutroRoboNaPosicao(Robo *listaRobos, Robo *roboAtual, Posicao pos) {
    Robo *r = listaRobos;
    while (r != NULL) {
        if (r != roboAtual && r->posicao.linha == pos.linha && r->posicao.coluna == pos.coluna) {
            return true;
        }
        r = r->next;
    }
    return false;
}

void robosAtualizarContinuo(Jogo *jogo, float deltaTime) {
    if (jogo == NULL) return;

    if (jogo->jogador.tempoCurtoCircuito > 0.0f) {
        jogo->jogador.tempoCurtoCircuito -= deltaTime;
        if (jogo->jogador.tempoCurtoCircuito < 0.0f) {
            jogo->jogador.tempoCurtoCircuito = 0.0f;
        }
    }

    bool lanternaAtiva = (jogo->jogador.tempoLanterna > 0.0f && jogo->jogador.bateria > 0);
    Posicao semExcecao = { -1, -1 };

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        if (atual->estado == ESTADO_DESLIGADO) {
            atual = atual->next;
            continue;
        }

        if (atual->tempoParalisado > 0.0f) {
            atual->tempoParalisado -= deltaTime;
            if (atual->tempoParalisado < 0.0f) atual->tempoParalisado = 0.0f;
            atual = atual->next;
            continue;
        }

        if (atual->tipo == ROBO_PERSEGUIDOR || atual->tipo == ROBO_INTERCEPTADOR ||
            (atual->tipo == ROBO_CINZA && jogo->andarAtual == 1)) {
            if (lanternaAtiva) {
                atual->estado = ESTADO_FUGINDO;
            } else {
                atual->estado = ESTADO_NORMAL;
            }
        } else {
            atual->estado = ESTADO_NORMAL;
        }

        if (atual->tipo == ROBO_CINZA && jogo->andarAtual == 2) {
            destruirBateriaNaPosicao(&jogo->itens, atual->posicao);
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
            }
            else if (atual->tipo == ROBO_TEMPORIZADOR) {
                if (jogo->professor.posicao.linha >= 0 && jogo->professor.posicao.coluna >= 0) {
                    Posicao passoRoxo = calcularProximoPassoBFS(&jogo->mapa, atual->posicao, jogo->professor.posicao);
                    if (casaLivreParaRobo(&jogo->mapa, passoRoxo.linha, passoRoxo.coluna, semExcecao)) {
                        proximaCasa = passoRoxo;
                    }
                }
            }
            else if (atual->tipo == ROBO_CINZA && jogo->andarAtual == 2) {
                Posicao alvoBateria;
                if (buscarBateriaMaisProxima(jogo->itens, atual->posicao, &alvoBateria)) {
                    proximaCasa = calcularProximoPassoBFS(&jogo->mapa, atual->posicao, alvoBateria);
                } else {
                    proximaCasa = calcularProximoPassoBFS(&jogo->mapa, atual->posicao, jogo->jogador.posicao);
                }
            }
            else {
                Posicao alvo = jogo->jogador.posicao; 
                int distAluno = abs(atual->posicao.linha - jogo->jogador.posicao.linha) +
                                abs(atual->posicao.coluna - jogo->jogador.posicao.coluna);

                if (distAluno > 3) {
                    Posicao alvoProjetado = jogo->jogador.posicao;

                    if (atual->tipo == ROBO_INTERCEPTADOR) {
                        if (jogo->jogador.direcao == DIR_CIMA) alvoProjetado.linha -= 2;
                        else if (jogo->jogador.direcao == DIR_BAIXO) alvoProjetado.linha += 2;
                        else if (jogo->jogador.direcao == DIR_ESQUERDA) alvoProjetado.coluna -= 2;
                        else if (jogo->jogador.direcao == DIR_DIREITA) alvoProjetado.coluna += 2;
                    } else if (atual->tipo == ROBO_CINZA && jogo->andarAtual != 2) {
                        if (jogo->jogador.direcao == DIR_CIMA) alvoProjetado.linha += 2;
                        else if (jogo->jogador.direcao == DIR_BAIXO) alvoProjetado.linha -= 2;
                        else if (jogo->jogador.direcao == DIR_ESQUERDA) alvoProjetado.coluna += 2;
                        else if (jogo->jogador.direcao == DIR_DIREITA) alvoProjetado.coluna -= 2;
                    }

                    if (casaLivreParaRobo(&jogo->mapa, alvoProjetado.linha, alvoProjetado.coluna, semExcecao)) {
                        alvo = alvoProjetado;
                    }
                }

                proximaCasa = calcularProximoPassoBFS(&jogo->mapa, atual->posicao, alvo);

                if (proximaCasa.linha == atual->posicao.linha && proximaCasa.coluna == atual->posicao.coluna) {
                    proximaCasa = calcularProximoPassoBFS(&jogo->mapa, atual->posicao, jogo->jogador.posicao);
                }
            }

            if (proximaCasa.linha != atual->posicao.linha || proximaCasa.coluna != atual->posicao.coluna) {
                if (temOutroRoboNaPosicao(jogo->robos, atual, proximaCasa)) {
                    int dL[4] = { -1, 1, 0, 0 };
                    int dC[4] = { 0, 0, -1, 1 };
                    bool desviou = false;

                    for (int i = 0; i < 4; i++) {
                        Posicao viz = { atual->posicao.linha + dL[i], atual->posicao.coluna + dC[i] };
                        if (casaLivreParaRobo(&jogo->mapa, viz.linha, viz.coluna, semExcecao) && 
                            !temOutroRoboNaPosicao(jogo->robos, atual, viz)) {
                            proximaCasa = viz;
                            desviou = true;
                            break;
                        }
                    }
                    if (!desviou) {
                        proximaCasa = atual->posicao;
                    }
                }
            }

            atual->posicaoAnterior = atual->posicao;
            atual->posicao = proximaCasa;
        }

        float dx = (float)atual->posicao.coluna - atual->posicaoReal.x;
        float dy = (float)atual->posicao.linha - atual->posicaoReal.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 0.001f) {
            float vel = atual->velocidade;
            if (vel <= 0.0f) vel = 4.0f;

            if (atual->estado == ESTADO_FUGINDO) {
                vel = vel * 0.85f;
            } else if (atual == roboQueBateu && passosRecuo > 0) {
                vel = vel * 1.35f;
            }

            float passo = vel * deltaTime;
            if (passo > dist) passo = dist;

            atual->direcao = (Vector2){ dx / dist, dy / dist };
            atual->posicaoReal.x += atual->direcao.x * passo;
            atual->posicaoReal.y += atual->direcao.y * passo;
        }

        atual = atual->next;
    }
}

void desenharRobos(Jogo *jogo, Vector2 origem, int tamanhoTile) {
    if (jogo == NULL) return;

    Robo *r = jogo->robos;
    while (r != NULL) {
        if (r->estado == ESTADO_DESLIGADO) {
            r = r->next;
            continue;
        }

        int idxTex = obterIndiceDirecaoRobo(r->direcao);
        float escalaX = 1.0f;
        if (r->direcao.x > 0) escalaX = -1.0f;

        Texture2D tex;
        if (r->tipo == ROBO_TEMPORIZADOR) {
            tex = jogo->texRoboTemporizador[idxTex];
        } else if (r->tipo == ROBO_PERSEGUIDOR) {
            tex = jogo->texRoboPerseguidor[idxTex];
        } else if (r->tipo == ROBO_CINZA) {
            if (idxTex == 1 && ((int)(GetTime() * 4.0) % 2 == 1)) {
                tex = jogo->texRoboCinza[4];
            } else {
                tex = jogo->texRoboCinza[idxTex];
            }
        } else {
            tex = jogo->texRoboInterceptador[idxTex];
        }

        if (tex.id > 0) {
            Vector2 posTela = {
                origem.x + r->posicaoReal.x * (float)tamanhoTile,
                origem.y + r->posicaoReal.y * (float)tamanhoTile
            };

            Rectangle srcRec = { 0.0f, 0.0f, (float)tex.width * escalaX, (float)tex.height };
            Rectangle destRec = { posTela.x, posTela.y, (float)tamanhoTile, (float)tamanhoTile };
            Vector2 origin = { 0.0f, 0.0f };

            Color tint = WHITE;
            if (r->estado == ESTADO_FUGINDO) tint = SKYBLUE;

            DrawTexturePro(tex, srcRec, destRec, origin, 0.0f, tint);
        }

        r = r->next;
    }
}

void checarVitoriaDerrotaRobos(Jogo *jogo) {
    if (jogo == NULL) return;

    if (jogo->jogador.tempoInvulneravel > 0.0f) {
        jogo->jogador.tempoInvulneravel -= GetFrameTime();
        if (jogo->jogador.tempoInvulneravel < 0.0f) {
            jogo->jogador.tempoInvulneravel = 0.0f;
        }
    }

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        if (atual->estado == ESTADO_DESLIGADO) {
            atual = atual->next;
            continue;
        }

        if (atual->tipo == ROBO_TEMPORIZADOR && !jogo->professor.ajudado && !jogo->jogador.temPendrive) {
            float dxProf = atual->posicaoReal.x - (float)jogo->professor.posicao.coluna;
            float dyProf = atual->posicaoReal.y - (float)jogo->professor.posicao.linha;
            float distProf = sqrtf(dxProf * dxProf + dyProf * dyProf);

            bool mesmaCasaProf = (atual->posicao.linha == jogo->professor.posicao.linha &&
                                  atual->posicao.coluna == jogo->professor.posicao.coluna);

            if (distProf < 0.8f || mesmaCasaProf) {
                jogo->telaAtual = TELA_GAME_OVER;
                return;
            }
        }

        if (atual->tempoParalisado <= 0.0f && jogo->jogador.tempoInvulneravel <= 0.0f && roboQueBateu == NULL) {
            float dx = atual->posicaoReal.x - (float)jogo->jogador.posicao.coluna;
            float dy = atual->posicaoReal.y - (float)jogo->jogador.posicao.linha;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist <= 0.65f) {
                if (atual->tipo == ROBO_CINZA && jogo->andarAtual == 3) {
                    jogo->jogador.tempoCurtoCircuito = 4.0f;
                }

                if (jogo->jogador.bateria > 0) {
                    jogo->jogador.bateria = 0;
                } else {
                    jogo->jogador.vidas--;
                    if (jogo->jogador.vidas <= 0) {
                        jogo->jogador.vidas = 0;
                        jogo->telaAtual = TELA_GAME_OVER;
                        return;
                    }
                }

                jogo->jogador.tempoInvulneravel = 3.0f;

                if (atual->tipo == ROBO_TEMPORIZADOR) {
                    atual->tempoParalisado = 5.0f;
                } else {
                    roboQueBateu = atual;
                    passosRecuo = 5;
                    atual->posicaoAnterior = atual->posicao;
                    atual->posicao = calcularPassoFuga(&jogo->mapa, atual, jogo->jogador.posicao);
                }
            }
        }

        atual = atual->next;
    }
}