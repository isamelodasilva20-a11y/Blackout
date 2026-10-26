#ifndef JOGO_H
#define JOGO_H

typedef enum { MENU, JOGANDO, PAUSA, VITORIA, GAME_OVER, RANKING } EstadoJogo;
typedef enum { ROBO_ALEATORIO, ROBO_PERSEGUIDOR, ROBO_INTERCEPTADOR } TipoRobo;
typedef enum { ESTADO_NORMAL, ESTADO_FUGINDO, ESTADO_DESLIGADO } EstadoRobo;

typedef struct {
    int linha;
    int coluna;
} Posicao;

typedef struct {
    int linhas;
    int colunas;
    char **matriz;
} Mapa;

typedef struct {
    Posicao pos;
    Posicao pos_inicial;
    int vidas;
    int pontos;
    int bateria;
    int ultima_direcao;
} Jogador;

typedef struct {
    Posicao pos;
    int entregou;
} Professor;

typedef struct Item {
    Posicao pos;
    int tipo;
    struct Item *proximo;
} Item;

typedef struct Robo {
    Posicao pos;
    Posicao pos_inicial;
    TipoRobo tipo;
    EstadoRobo estado;
    struct Robo *proximo;
} Robo;

typedef struct {
    Mapa mapa;
    Jogador jogador;
    Professor professor;
    Item *itens;
    Robo *robos;         
    int andar_atual;
    EstadoJogo tela_atual;
} Jogo;

#endif 