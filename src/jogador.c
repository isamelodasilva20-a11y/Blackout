#include <stddef.h>
#include "jogo.h"

void movimentar_jogador(Jogador *jogador, Mapa *mapa, Direcao direcao){
    if(jogador == NULL || mapa == NULL || mapa->matriz == NULL) return;

    //posiçao atual
    int nova_linha = jogador->posicao.linha;
    int nova_coluna = jogador->posicao.coluna;

    jogador->direcao = direcao;

    switch (direcao){
        case DIR_CIMA: nova_linha--; break;
        case DIR_BAIXO: nova_linha++; break;
        case DIR_DIREITA: nova_coluna++; break;
        case DIR_ESQUERDA: nova_coluna--; break;
    }

    if(nova_linha>=0 && nova_linha<mapa->linhas && nova_coluna >= 0 && nova_coluna < mapa->colunas){
        if(mapa->matriz[nova_linha][nova_coluna] != '#'){
            jogador->posicao.linha = nova_linha;
            jogador->posicao.coluna = nova_coluna;

            // ALTERA O PASSO: se estava no frame 1 vai pro 2, se estava no 2 (ou 0) vai pro 1
            if (jogador->frame == 1) {
                jogador->frame = 2;
            } else {
                jogador->frame = 1;
            }
        }
    }
}   