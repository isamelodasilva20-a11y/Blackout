
#ifndef JOGO_H
#define JOGO_H

#include "raylib.h"

// Enumerações para o módulo e para a máquina de telas
typedef enum { MENU, JOGANDO, PAUSA, VITORIA, GAME_OVER, RANKING } EstadoJogo;
typedef enum { ROBO_ALEATORIO, ROBO_PERSEGUIDOR, ROBO_INTERCEPTADOR } TipoRobo;
typedef enum { ESTADO_NORMAL, ESTADO_FUGINDO, ESTADO_DESLIGADO } EstadoRobo;

// 1. Posição no mapa (linha e coluna)
typedef struct {
    int linha;
    int coluna;
} Posicao;

// 2. Mapa do andar
typedef struct {
    int linhas;
    int colunas;
    char **matriz;
} Mapa;

// 3. Jogador
typedef struct {
    Posicao pos;
    Posicao pos_inicial;
    int vidas;
    int pontos;
    int bateria;
    int ultima_direcao;
} Jogador;

// Estrutura do Professor (que segura o Pen Drive)
typedef struct {
    Posicao pos;
    int entregou;
} Professor;

// 4. Lista encadeada de Itens
typedef struct Item {
    Posicao pos;
    int tipo;
    struct Item *proximo;
} Item;

// 5. Lista encadeada de Robôs 
typedef struct Robo {
    Posicao pos;
    Posicao pos_inicial;
    TipoRobo tipo;
    EstadoRobo estado;
    struct Robo *proximo;
} Robo;

// 6. Estrutura geral do Jogo
typedef struct {
    Mapa mapa;
    Jogador jogador;
    Professor professor;
    Item *itens;
    Robo *robos;         // Início da lista de robôs
    int andar_atual;
    EstadoJogo tela_atual;
} Jogo;

#endif // JOGO_H