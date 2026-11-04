#ifndef JOGO_H
#define JOGO_H
#include <raylib.h>
#include <stdbool.h> // tipo bool (true/false), usado em temPendrive e ajudado

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
    char **matriz;     // matriz[linha][coluna]: '#' parede, '.' corredor, ' ' sala, 'M' mesa, 'C' computador (legenda em mapa.h)
    Posicao saida;     // escada para o proximo andar ('S' no arquivo); linha -1 se o andar nao tiver
} Mapa;

#define VIDAS_INICIAIS 3 // vidas no comeco da partida (coracoes do HUD)

typedef struct {
    Posicao posicao;          
    Posicao posicaoInicial;   
    Direcao direcao;           
    int vidas;            
    int pontos;           
    int bateria;
    int frame;
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
    const char *pergunta;
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

typedef enum {
    TELA_NORMAL,
    TELA_QUIZ
} TelaJogo;

typedef enum {
    FEEDBACK_NENHUM,
    FEEDBACK_ACERTOU,
    FEEDBACK_ERROU
} EstadoFeedback;

// --- 7. ESTADO GERAL DO JOGO ---
typedef struct {
    Mapa mapa;
    Jogador jogador;
    Texture2D texturaJogador; 
    Item *itens;          
    Robo *robos;     
    Professor professor;  
    int andarAtual;
    TelaJogo telaAtual;
    EstadoFeedback estadoFeedback;
    double tempoInicioFeedback;
    int modoVisao;        
    float tempoJogo;      // cronometro: segundos desde o inicio da partida
    int totalItensAndar;  // quantas baterias o andar atual tinha ao carregar (para o HUD)
} Jogo;

typedef struct {
    // ... os teus outros campos (mapa, jogador, robos, etc) ...
    
    Texture2D texRoboInterceptador;
    Texture2D texRoboAleatorio;
    Texture2D texRoboPerseguidor;
} Jogo;

#endif