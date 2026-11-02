#include <stddef.h>
#include "raylib.h"
#include "jogo.h"

void desenharMapa2D(Mapa *mapa, int tamanhoCasa) {
    if(mapa == NULL || mapa->matriz == NULL) return;

    for(int i = 0; i < mapa->linhas; i++){
        for(int j = 0; j < mapa->colunas; j++){
            if(mapa->matriz[i][j] == '#'){
                DrawRectangle(j * tamanhoCasa, i * tamanhoCasa, tamanhoCasa, tamanhoCasa, DARKBLUE);
            }
        }
    }
}

#include "render_2d.h"

void desenharJogador2D(Jogo *jogo, int tamanhoCasa) {
    if (jogo == NULL || jogo->texturaJogador.id == 0) return;

    float posX = jogo->jogador.posicao.coluna * tamanhoCasa;
    float posY = jogo->jogador.posicao.linha * tamanhoCasa;

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




// Desenha a escada (saida do andar) como um quadrado na casa do 'S'
void desenharSaida2D(Mapa *mapa, int tamanhoCasa, bool liberada) {
    if(mapa == NULL || mapa->saida.linha < 0) return; // andar sem escada

    // Escada fica cinza enquanto ainda faltam baterias e verde quando o jogador pode subir
    Color cor = liberada ? GREEN : DARKGRAY;

    // Converte linha/coluna em pixels; o +2 / -4 deixa uma borda de 2px para o
    // quadrado nao encostar nas paredes vizinhas
    DrawRectangle(mapa->saida.coluna * tamanhoCasa + 2, mapa->saida.linha * tamanhoCasa + 2,
                  tamanhoCasa - 4, tamanhoCasa - 4, cor);
}
