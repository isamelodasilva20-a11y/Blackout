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

void desenharJogador2D(Jogador *jogador, int tamanhoCasa) {
    if(jogador == NULL) return;

    // Calcula as coordenadas em pixels no centro da casa
    float centroX = (jogador->posicao.coluna * tamanhoCasa) + (tamanhoCasa / 2.0f);
    float centroY = (jogador->posicao.linha * tamanhoCasa) + (tamanhoCasa / 2.0f);
    float raio = (tamanhoCasa / 2.0f) - 3.0f;

    DrawCircle((int)centroX, (int)centroY, raio, YELLOW);
}
