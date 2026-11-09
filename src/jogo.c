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
    jogo->texRoboInterceptador[0] = LoadTexture("imagens/robo_laranja.png");           // Cima (Costas)
    jogo->texRoboInterceptador[1] = LoadTexture("imagens/robo_laranja_frente.png");    // Baixo (Frente)
    jogo->texRoboInterceptador[2] = LoadTexture("imagens/robo_laranja_lado_esq.png"); // Esquerda
    jogo->texRoboInterceptador[3] = LoadTexture("imagens/robo_laranja_lado_dir.png"); // Direita

    // Perseguidor (Vermelho)
    jogo->texRoboPerseguidor[0]   = LoadTexture("imagens/robo_vermelho_costas.png");    // Cima (Costas)
    jogo->texRoboPerseguidor[1]   = LoadTexture("imagens/robo_vermelho_frente.png");    // Baixo (Frente)
    jogo->texRoboPerseguidor[2]   = LoadTexture("imagens/robo_vermelho_lado_dir.png"); // Esquerda
    jogo->texRoboPerseguidor[3]   = LoadTexture("imagens/robo_vermelho_lado_dir.png"); // Direita

    // Temporizador (Roxo)
    jogo->texRoboTemporizador[0] = LoadTexture("imagens/robo_roxo_costas.png");         
    jogo->texRoboTemporizador[1] = LoadTexture("imagens/robo_roxo_lado.png");           
    jogo->texRoboTemporizador[2] = LoadTexture("imagens/robo_roxo_lado.png");           
    jogo->texRoboTemporizador[3] = LoadTexture("imagens/robo_roxo_lado.png");
}

// 2. Leitura dos caracteres do mapa para adicionar os robôs com a velocidade ajustada ao andar
void jogoCarregarMapaRobos(Jogo *jogo, char mapaTexto[100][100], int linhas, int colunas) {
    if (jogo == NULL) return;

    for (int l = 0; l < linhas; l++) {
        for (int c = 0; c < colunas; c++) {
            char caractere = mapaTexto[l][c];
            Posicao posAtual = { l, c };

            if (caractere == 'I') {
                robosAdicionar(&jogo->robos, posAtual, ROBO_INTERCEPTADOR, jogo->andarAtual);
            } else if (caractere == 'P') {
                robosAdicionar(&jogo->robos, posAtual, ROBO_PERSEGUIDOR, jogo->andarAtual);
            } else if (caractere == 'T' || caractere == 'R') {
                robosAdicionar(&jogo->robos, posAtual, ROBO_TEMPORIZADOR, jogo->andarAtual);
            }
        }
    }
}

// 3. Atualização contínua de colisões
void jogoTick(Jogo *jogo) {
    if (jogo == NULL) return;

    checarVitoriaDerrotaRobos(jogo);
}

// 4. Descarregar as texturas da memória ao fechar o jogo
void jogoDescarregar(Jogo *jogo) {
    if (jogo == NULL) return;

    for (int i = 0; i < 4; i++) {
        UnloadTexture(jogo->texRoboInterceptador[i]);
        UnloadTexture(jogo->texRoboPerseguidor[i]);
        UnloadTexture(jogo->texRoboTemporizador[i]);
    }
}