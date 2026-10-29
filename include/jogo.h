#ifndef JOGO_H
#define JOGO_H

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
} Jogador;


typedef enum {
    ITEM_PENDRIVE,
    ITEM_LANTERNA
} TipoItem;

typedef struct Item {
    Posicao posicao;
    TipoItem tipo;
    struct Item *next;
} Item;


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
    int andarAtual;
    int telaAtual;        
    int modoVisao;        
} Jogo;

#endif