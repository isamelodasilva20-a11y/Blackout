#include "raylib.h"
#include "jogo.h"
#include "jogador.h"
#include "render_2d.h"
#include "mapa.h"
#include "memoria.h"



int main(void) {
    InitWindow(1024, 640, "Blackout");
    SetTargetFPS(60);

    Jogo jogo = {0}; //estado do jogo

    if (!mapaCarregar(&jogo, "andares/andar1.txt")) {
        CloseWindow();
        return 1;
    }

    float acumuladorTempo = 0.0f;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))    movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_CIMA);
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))  movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_BAIXO);
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_ESQUERDA);
        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_DIREITA);


        acumuladorTempo += GetFrameTime();
        if (acumuladorTempo >= 0.05f) {
            acumuladorTempo -= 0.05f;
        }

        BeginDrawing();
            ClearBackground(BLACK);

            desenharMapa2D(&jogo.mapa, 24);
            desenharJogador2D(&jogo.jogador, 24);

        EndDrawing();
    }

    mapaLiberar(&jogo.mapa);
    memRelatorio();
    CloseWindow();
    return 0;
}
