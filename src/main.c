#include "raylib.h"
#include "jogo.h"
#include "jogador.h"
#include "render_2d.h"
#include "mapa.h"
#include "memoria.h"
#include "itens.h"



int main(void) {
    InitWindow(1024, 640, "Blackout");
    SetTargetFPS(60);

    Jogo jogo = {0}; //estado do jogo

    if (!mapaCarregar(&jogo, "andares/andar1.txt")) {
        CloseWindow();
        return 1;
    }

    Item *lista_itens = NULL;
    adicionar_item(&lista_itens, (Posicao){ .linha = 3, .coluna = 3}, ITEM_LANTERNA);
    adicionar_item(&lista_itens, (Posicao){ .linha = 8, .coluna = 7}, ITEM_PENDRIVE);

    float acumuladorTempo = 0.0f;
    int venceu = 0;

        while (!WindowShouldClose()) {
            if(!venceu){
                if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))    movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_CIMA);
                if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))  movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_BAIXO);
                if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_ESQUERDA);
                if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_DIREITA);

                coletar_item(&lista_itens, &jogo.jogador);

                if(verificar_vitoria(lista_itens)){
                    venceu = 1;
                }
            }

            acumuladorTempo += GetFrameTime();
            if (acumuladorTempo >= 0.05f) {
                acumuladorTempo -= 0.05f;
            }

            BeginDrawing();
                ClearBackground(BLACK);
                desenharMapa2D(&jogo.mapa, 24);
                desenharJogador2D(&jogo.jogador, 24);
                desenhar_item(lista_itens, 24);

                if(venceu){
                    DrawRectangle(0, 0, 1024, 640, Fade(BLACK, 0.7f)); // Fundo semi-transparente
                    DrawText("VOCE VENCEU! TODOS OS ITENS COLETADOS!", 200, 300, 30, GREEN);
                    DrawText("Pressione ESC para sair", 380, 350, 20, WHITE);
                }

            EndDrawing();
        }
    

    liberar_todos_itens(&lista_itens);
    mapaLiberar(&jogo.mapa);

    memRelatorio();
    CloseWindow();
    return 0;
}
