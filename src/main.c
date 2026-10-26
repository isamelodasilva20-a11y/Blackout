#include "raylib.h"
#include "jogo.h"
#include "memoria.h"

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 450;
    int *teste = memAlocar(sizeof(int));
    memLiberar(teste);

    InitWindow(screenWidth, screenHeight, "Blackout - CESAR School");
    SetTargetFPS(60);

    Jogo jogo = {0};
    jogo.tela_atual = JOGANDO;

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(BLACK);

            if (jogo.tela_atual == JOGANDO) {
                DrawText("Ambiente OK! Pressione ESC para sair.", 190, 200, 20, LIGHTGRAY);
            }
        EndDrawing();
    }

    CloseWindow();
    memRelatorio();

    return 0;
}