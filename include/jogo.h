#ifndef JOGO_H
#define JOGO_H
#include <stdbool.h>

typedef enum {
    DIR_CIMA,
    DIR_BAIXO,
    DIR_ESQUERDA,
    DIR_DIREITA
} Direcao;

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
    Posicao posicao;          
    Posicao posicaoInicial;   
    Direcao direcao;           
    int vidas;            
    int pontos;           
    int bateria;
    bool temPendrive;         
} Jogador;


typedef enum {
    ITEM_BATERIA
} TipoItem;

typedef struct Item {
    Posicao posicao;
    TipoItem tipo;
    struct Item *next;
} Item;

typedef struct {
    Posicao posicao;
    const char *nome;
    const char *pegunta;
    const char *resposta;
    bool ajudado;
}Professor;



typedef enum {
    ROBO_ALEATORIO,     
    ROBO_PERSEGUIDOR,   
    ROBO_INTERCEPTADOR  
} TipoRobo;

typedef enum {
    ESTADO_NORMAL,
    ESTADO_FUGINDO,
    ESTADO_DESLIGADO
} EstadoRobo;

typedef struct Robo {
    Posicao posicao;
    Posicao posicaoInicial;
    TipoRobo tipo;
    EstadoRobo estado;
    struct Robo *next;
} Robo;

// --- 7. ESTADO GERAL DO JOGO ---
typedef struct {
    Mapa mapa;
    Jogador jogador;
    Item *itens;          
    Robo *robos;     
    Professor professor;  
    int andarAtual;
    int telaAtual;        
    int modoVisao;        
} Jogo;

#endif