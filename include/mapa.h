#ifndef MAPA_H
#define MAPA_H

#include "jogo.h"

/*
 * Formato dos arquivos andares/andarN.txt
 *
 * Primeira linha: "<linhas> <colunas>" (hoje todos os andares sao 25 40).
 * O tamanho de cada casa na tela e calculado pelo hudLayoutMapa (hud.c)
 * para o mapa ocupar o maximo da janela, qualquer que seja o tamanho dela.
 * Depois vem o desenho do mapa, um caractere por casa:
 *
 *   #   parede (bloqueia o movimento)
 *   .   piso de corredor
 *   ' ' (espaco) piso de sala - tambem e andavel, mas desenhado diferente
 *   M   mesa (movel: bloqueia o movimento)
 *   C   computador em cima de uma mesa (movel: bloqueia o movimento)
 *   P   onde o jogador comeca no andar
 *   S   escada para o proximo andar (saida)
 *   I   bateria (item coletavel)
 *   F   professor do andar (quem libera a escada depois do quiz)
 *   R X Z  robos (reservados para a parte dos robos; por enquanto viram piso)
 *   L   lanterna (ignorada, a lanterna agora e fixa)
 *
 * As letras de objetos (P, S, I, F, R, X, Z, L) viram piso na matriz: o tipo
 * de piso (corredor ou sala) e copiado da casa a esquerda.
 *
 * Os andares se ligam: o S de um andar fica na mesma posicao do P do proximo,
 * como se o jogador tivesse acabado de subir a escada.
 *   andar 1: P (1,1)   -> S (22,38)   F (20,5)    2 baterias
 *   andar 2: P (22,38) -> S (1,1)     F (1,37)    3 baterias  (laboratorios)
 *   andar 3: P (1,1)   -> S (12,19)   F (23,37)   4 baterias  (sala de servidores no centro)
 */

#define TOTAL_ANDARES 3 // quantos arquivos andarN.txt o jogo percorre ate a vitoria



// Funcoes de desenho 2D: origem = canto superior esquerdo do mapa na tela, em
// pixels. Assim o mesmo codigo desenha o mapa grande (origem 0,0 e casas de
// 24px) e o minimapa do modo 3D (canto da tela e casas pequenas).
void desenharMapa2D(struct Mapa *mapa, Vector2 origem, int tamanhoCasa);

// liberada = true quando o professor ja foi ajudado (escada verde)
void desenharSaida2D(struct Mapa *mapa, Vector2 origem, int tamanhoCasa, bool liberada);

// true se o jogador NAO pode entrar na casa: fora do mapa, parede ou movel
bool mapaCasaBloqueada(struct Mapa *mapa, int linha, int coluna);

// Le um arquivo de mapa para dentro de jogo->mapa. Tambem posiciona o jogador
// (P) e o professor (F), guarda a escada (S) e cria as baterias (I) na lista
// jogo->itens. Retorna 1 se deu certo e 0 se deu erro.
int  mapaCarregar(struct  Jogo *jogo, const char *arquivo);

// Troca para o andar indicado (1 a TOTAL_ANDARES): libera o mapa e os itens
// do andar atual, carrega andares/andarN.txt e reinicia o professor (nova
// pergunta, ainda nao ajudado) e o pendrive do jogador. Retorna 1 se deu certo.
int  mapaCarregarAndar(struct Jogo *jogo, int andar);

// Devolve para o sistema toda a memoria da matriz do mapa.
void mapaLiberar(struct Mapa *mapa);

#endif
