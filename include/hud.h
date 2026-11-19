#ifndef HUD_H
#define HUD_H

#include "raylib.h"

// Avisa o compilador que estas structs existem, SEM criar um novo typedef
struct Jogo;
struct Mapa;

#define LARGURA_BASE     1024
#define ALTURA_BASE      640
#define HUD_ALTURA_BASE  60  

float hudEscala(void);

// Ajudantes de texto usados pelo HUD, pelas telas e pela tela inicial
// (ficam no hud.c para nao existir uma copia de cada em cada arquivo)
void textoCentralizado(const char *texto, int centroX, int y, int fonte, Color cor);
const char *formatarTempo(float segundos); // "MM:SS" num buffer temporario

int hudAlturaBarra(void);

// Usamos "struct Mapa" e "struct Jogo" aqui para evitar qualquer conflito
void hudLayoutMapa(struct Mapa *mapa, Vector2 *origem, int *tamanhoCasa);

void desenharHUD(struct Jogo *jogo);

// O que o jogador escolheu clicando num botao das telas de pausa, game over
// e vitoria. As teclas (P, ESC, R, Q) sao tratadas no main.c; as funcoes de
// desenho so informam o clique, porque so elas sabem onde o botao esta.
typedef enum {
    ACAO_NENHUMA,    // nenhum botao foi clicado neste quadro
    ACAO_CONTINUAR,  // pausa: voltar ao jogo
    ACAO_REINICIAR,  // game over: tentar de novo (mesmo que a tecla R)
    ACAO_MENU,       // pausa e game over: voltar para a tela inicial
    ACAO_SAIR        // fechar o jogo (com a limpeza do main.c)
} AcaoTela;

// Tela de vitoria com o ranking e o botao SAIR (ESC ou clique)
AcaoTela desenharTelaVitoria(struct Jogo *jogo, EntradaRanking ranking[], int totalRanking);

// Pausa com o resumo da partida e os botoes CONTINUAR (P/ESC) e MENU (Q,
// volta para a tela inicial)
AcaoTela desenharTelaPausa(struct Jogo *jogo);

// Tela inicial com a historia do jogo (telainicial.c). Devolve true quando o
// jogador aperta ENTER (comecar) e false quando ele quer sair (ESC ou o X da
// janela), para o main.c pular a partida e ir direto para a limpeza.
bool telaInicialMostrar(char *nomeJogador, int maxChars);

// Game over animado com o resumo da partida (andar, tempo e pendrives do jogo)
// e os botoes TENTAR DE NOVO (R) e MENU (ESC, volta para a tela inicial)
AcaoTela desenharTelaGameOver(struct Jogo *jogo);

// Efeito da luz acendendo no comeco da partida (t = segundos desde o comeco).
// Desenha um preto que vai sumindo, com piscos; depois de ~1,3 s nao faz nada.
void desenharLuzAcendendo(float t);
#endif