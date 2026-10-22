#include "raylib.h"
#include "jogo.h"

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Blackout - CESAR School");
    SetTargetFPS(60);

    Jogo jogo = {0};
    jogo.tela_atual = JOGANDO;

    while (!WindowShouldClose()) {
        
        // --- Atualizações de Lógica (Update) ---
        // (Será preenchido nos próximos itens do backlog)

        // --- Desenho na Tela (Draw) ---
        BeginDrawing();
            ClearBackground(BLACK);

            if (jogo.tela_atual == JOGANDO) {
                DrawText("Ambiente OK! Pressione ESC para sair.", 190, 200, 20, LIGHTGRAY);
            }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}