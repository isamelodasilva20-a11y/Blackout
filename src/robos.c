#include "robos.h"
#include "memoria.h"
#include <stdlib.h>

void robosAdicionar(Robo **cabeca, Posicao posicao, TipoRobo tipo) {
    Robo *novo = (Robo *)memAlocar(sizeof(Robo));
    if (novo == NULL) return;
    
    novo->posicao = posicao;
    novo->posicaoInicial = posicao;
    novo->tipo = tipo;
    novo->estado = ESTADO_NORMAL;
    novo->next = *cabeca;
    *cabeca = novo;
}


void robosAtualizar(Jogo *jogo) {
    if (jogo == NULL) return;

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        
        if (atual->estado != ESTADO_DESLIGADO) {
            Posicao novaPos = atual->posicao;
            Posicao alvo = jogo->jogador.posicao;

            // Se a lanterna estiver ativa, todos os robôs movem-se de forma aleatória (fogem/ficam confusos)
            if (atual->estado == ESTADO_FUGINDO) {
                int dir = rand() % 4;
                if (dir == 0) novaPos.linha--;
                else if (dir == 1) novaPos.linha++;
                else if (dir == 2) novaPos.coluna--;
                else if (dir == 3) novaPos.coluna++;
            } 
            else {
                // Comportamento normal de caça
                switch (atual->tipo) {
                    
                    case ROBO_INTERCEPTADOR: {
                        int dist = 2; // Tenta adivinhar 2 casas à frente
                        switch (jogo->jogador.direcao) {
                            case DIR_CIMA:     alvo.linha -= dist; break;
                            case DIR_BAIXO:    alvo.linha += dist; break;
                            case DIR_ESQUERDA: alvo.coluna -= dist; break;
                            case DIR_DIREITA:  alvo.coluna += dist; break;
                        }

                        // Aproxima-se do alvo
                        if (novaPos.linha < alvo.linha) novaPos.linha++;
                        else if (novaPos.linha > alvo.linha) novaPos.linha--;
                        else if (novaPos.coluna < alvo.coluna) novaPos.coluna++;
                        else if (novaPos.coluna > alvo.coluna) novaPos.coluna--;
                        break;
                    }

                    case ROBO_ALEATORIO: {
                        // Calcula a distância Manhattan até o jogador
                        int distancia = abs(atual->posicao.linha - alvo.linha) + 
                                        abs(atual->posicao.coluna - alvo.coluna);
                        
                        if (distancia <= 5) {
                            // Se está perto (ex: 5 casas), persegue diretamente
                            if (novaPos.linha < alvo.linha) novaPos.linha++;
                            else if (novaPos.linha > alvo.linha) novaPos.linha--;
                            else if (novaPos.coluna < alvo.coluna) novaPos.coluna++;
                            else if (novaPos.coluna > alvo.coluna) novaPos.coluna--;
                        } else {
                            // Se está longe, anda aleatório
                            int dir = rand() % 4;
                            if (dir == 0) novaPos.linha--;
                            else if (dir == 1) novaPos.linha++;
                            else if (dir == 2) novaPos.coluna--;
                            else if (dir == 3) novaPos.coluna++;
                        }
                        break;
                    }

                    case ROBO_PERSEGUIDOR: {
                        // TODO: Implementar a fila e o BFS aqui (ou chamar uma função externa).
                        // Por enquanto, usa perseguição simples para não ficar parado:
                        if (novaPos.linha < alvo.linha) novaPos.linha++;
                        else if (novaPos.linha > alvo.linha) novaPos.linha--;
                        else if (novaPos.coluna < alvo.coluna) novaPos.coluna++;
                        else if (novaPos.coluna > alvo.coluna) novaPos.coluna--;
                        break;
                    }
                }
            }

            // Valida se o novo movimento é válido (dentro do mapa e não é parede)
            if (novaPos.linha >= 0 && novaPos.linha < jogo->mapa.linhas &&
                novaPos.coluna >= 0 && novaPos.coluna < jogo->mapa.colunas) {
                if (jogo->mapa.matriz[novaPos.linha][novaPos.coluna] != '#') {
                    atual->posicao = novaPos;
                }
            }
        }
        atual = atual->next;
    }
}

void checarColisoes(Jogo *jogo) {
    if (jogo == NULL) return;

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        if (atual->estado != ESTADO_DESLIGADO) {
            // Verifica se a posicao do jogador é a mesma do robo
            if (jogo->jogador.posicao.linha == atual->posicao.linha &&
                jogo->jogador.posicao.coluna == atual->posicao.coluna) {
                
                jogo->jogador.vidas--;
                
                // Reseta jogador e robos para a posicao inicial
                jogo->jogador.posicao = jogo->jogador.posicaoInicial;
                
                Robo *r = jogo->robos;
                while (r != NULL) {
                    r->posicao = r->posicaoInicial;
                    r = r->next;
                }
                break;
            }
        }
        atual = atual->next;
    }
}

void robosLiberar(Robo **cabeca) {
    if (cabeca == NULL || *cabeca == NULL) return;

    Robo *atual = *cabeca;
    while (atual != NULL) {
        Robo *proximo = atual->next;
        memLiberar(atual);
        atual = proximo;
    }
    *cabeca = NULL;
}
void desenharRobos(Jogo *jogo, Vector2 origem, int tamanhoTile) {
    if (jogo == NULL) return;

    Robo *atual = jogo->robos;
    while (atual != NULL) {
        if (atual->estado != ESTADO_DESLIGADO) {
            float posX = origem.x + (atual->posicao.coluna * tamanhoTile);
            float posY = origem.y + (atual->posicao.linha * tamanhoTile);

            Texture2D texturaAtual;
            Color corFiltro = (atual->estado == ESTADO_FUGINDO) ? BLUE : WHITE;

            if (atual->tipo == ROBO_INTERCEPTADOR) {
                texturaAtual = jogo->texRoboInterceptador;
            } else if (atual->tipo == ROBO_ALEATORIO) {
                texturaAtual = jogo->texRoboAleatorio;
            } else {
                texturaAtual = jogo->texRoboPerseguidor;
            }

            Rectangle origemTextura = { 0, 0, (float)texturaAtual.width, (float)texturaAtual.height };
            Rectangle destino = { posX, posY, (float)tamanhoTile, (float)tamanhoTile };
            Vector2 origemRotacao = { 0, 0 };

            DrawTexturePro(texturaAtual, origemTextura, destino, origemRotacao, 0.0f, corFiltro);
        }
        atual = atual->next;
    }
}