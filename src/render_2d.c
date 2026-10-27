#include <stddef.h>
#include "raylib.h"
#include "jogo.h"

void desenharJogador2D(Jogador *jogador, int tamanhoCasa) {
    if(jogador == NULL) return;

    // Calcula as coordenadas em pixels no centro da casa
    float centroX = (jogador->posicao.coluna * tamanhoCasa) + (tamanhoCasa / 2.0f);
    float centroY = (jogador->posicao.linha * tamanhoCasa) + (tamanhoCasa / 2.0f);
    float raio = (tamanhoCasa / 2.0f) - 3.0f;

    DrawCircle((int)centroX, (int)centroY, raio, YELLOW);
}
