#include "jogador.h"
#include "raylib.h"
#include "jogo.h"
#include <stddef.h>

//responsável pelos movimentos do jogador
void movimentar_jogador(Jogador *jogador, Mapa *mapa, Direcao direcao){
    if(jogador == NULL || mapa == NULL || mapa->matriz == NULL) return;

    //assume a posiçao atual, a proxima virá a partir do next dessa
    int nova_linha = jogador->posicao.linha;
    int nova_coluna = jogador->posicao.coluna;

    //define em qual direcão o jogador está indo
    jogador->direcao = direcao;

    //switch case é um tipo mais simples de condicional
    switch (direcao){
        //anda pela matriz de acordo com a direção
        case DIR_CIMA: nova_linha--; break;
        case DIR_BAIXO: nova_linha++; break;
        case DIR_DIREITA: nova_coluna++; break;
        case DIR_ESQUERDA: nova_coluna--; break;
    }
    //verifica se, apos a movimenação, a posição atual ainda está dentro dos limites do mapa
    if(nova_linha>=0 && nova_linha<mapa->linhas && nova_coluna >= 0 && nova_coluna < mapa->colunas){
        //verifica se não é uma parede
        if(mapa->matriz[nova_linha][nova_coluna] != '#'){
            jogador->posicao.linha = nova_linha;
            jogador->posicao.coluna = nova_coluna;

            // ALTERNA O PASSO: se estava no frame 1 vai pro 2, se estava no 2 (ou 0) vai pro 1
            if (jogador->frame == 1) {
                jogador->frame = 2;
            } else {
                jogador->frame = 1;
            }
        }
    }
}   


void desenharJogador2D(Jogo *jogo, int tamanhoCasa) {
    if (jogo == NULL || jogo->texturaJogador.id == 0) return;

    //converte a posicao da matris pra posição em pikel na imagem
    float posX = jogo->jogador.posicao.coluna * tamanhoCasa;
    float posY = jogo->jogador.posicao.linha * tamanhoCasa;
    //tamanho que será cortado na imagem
    float larguraFrame = (float)jogo->texturaJogador.width / 4.0f;
    float alturaFrame  = (float)jogo->texturaJogador.height / 3.0f;

    int linhaDirecao = 0;
    
    if (jogo->jogador.direcao == DIR_CIMA) {
        linhaDirecao = 0;
    } else if (jogo->jogador.direcao == DIR_BAIXO) {
        linhaDirecao = 1;
    } else if (jogo->jogador.direcao == DIR_ESQUERDA || jogo->jogador.direcao == DIR_DIREITA) {
        linhaDirecao = 2; // Linha do perfil
    }

    // Se a imagem original aponta para a ESQUERDA:
    // Mantém largura positiva para a ESQUERDA e NEGATIVA para a DIREITA (espelhar)
    float larguraSrc = larguraFrame;
    if (jogo->jogador.direcao == DIR_DIREITA) {
        larguraSrc = -larguraFrame;
    }

    Rectangle srcRec = {
        .x = (jogo->jogador.frame % 4) * larguraFrame,
        .y = linhaDirecao * alturaFrame,
        .width = larguraSrc,
        .height = alturaFrame
    };

    Rectangle destRec = { posX, posY, (float)tamanhoCasa, (float)tamanhoCasa };
    Vector2 origin = { 0.0f, 0.0f };

    DrawTexturePro(jogo->texturaJogador, srcRec, destRec, origin, 0.0f, WHITE);
}
