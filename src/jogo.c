#include <stddef.h>
#include <stdio.h>
#include "robos.h"
#include "raylib.h"
#include "jogo.h"


// 1. Inicializa e carrega as texturas das 4 direções de cada robô
void jogoCarregar(Jogo *jogo) {
    if (jogo == NULL) return;

    // --- CARREGAMENTO DAS TEXTURAS DOS ROBÔS ---
    // Interceptador (Laranja)
    jogo->texRoboInterceptador[0] = LoadTexture("imagens/robo_laranja_frente.png");   // Cima (Costas)
    jogo->texRoboInterceptador[1] = LoadTexture("imagens/robo_laranja.png");          // Baixo (Frente)
    jogo->texRoboInterceptador[2] = LoadTexture("imagens/robo_laranja_lado_esq.png"); // Esquerda
    jogo->texRoboInterceptador[3] = LoadTexture("imagens/robo_laranja_lado_dir.png"); // Direita

    // Perseguidor (Vermelho)
    jogo->texRoboPerseguidor[0]   = LoadTexture("imagens/robo_vermelho_costas.png");    // Cima (Costas)
    jogo->texRoboPerseguidor[1]   = LoadTexture("imagens/robo_vermelho_frente.png");    // Baixo (Frente)
    jogo->texRoboPerseguidor[2]   = LoadTexture("imagens/robo_vermelho_lado_dir.png"); // Esquerda
    jogo->texRoboPerseguidor[3]   = LoadTexture("imagens/robo_vermelho_lado_dir.png"); // Direita

    // Temporizador (Roxo)
    jogo->texRoboTemporizador[0] = LoadTexture("imagens/robo_roxo_costas.png");
    jogo->texRoboTemporizador[1] = LoadTexture("imagens/robo_transparente.png");
    jogo->texRoboTemporizador[2] = LoadTexture("imagens/robo_roxo_lado.png");
    jogo->texRoboTemporizador[3] = LoadTexture("imagens/robo_roxo_lado.png");

    // Cinza 
    // Cinza (indices: 0 = costas, 1 = frente, 2 = esquerda, 3 = direita, 4 = frente animado)
    jogo->texRoboCinza[0] = LoadTexture("imagens/robo_cinza_costas.png");
    jogo->texRoboCinza[1] = LoadTexture("imagens/robo_cinza_acenando.png");
    jogo->texRoboCinza[2] = LoadTexture("imagens/robo_cinza_esquerda_2.png");
    jogo->texRoboCinza[3] = LoadTexture("imagens/robo_cinza_direita.png");
    jogo->texRoboCinza[4] = LoadTexture("imagens/robo_cinza_acenando_2.png");
       
}   


// 2. Descarregar as texturas da memória ao fechar o jogo
void jogoDescarregar(Jogo *jogo) {
    if (jogo == NULL) return;

    for (int i = 0; i < 4; i++) {
        UnloadTexture(jogo->texRoboInterceptador[i]);
        UnloadTexture(jogo->texRoboPerseguidor[i]);
        UnloadTexture(jogo->texRoboTemporizador[i]);
        UnloadTexture(jogo->texRoboCinza[i]);
    }
    UnloadTexture(jogo->texRoboCinza[4]);
}