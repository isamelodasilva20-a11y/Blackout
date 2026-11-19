#ifndef MAPA_H
#define MAPA_H

#include "estrutura.h"

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
 *   =   tapete com a logo do CESAR - um retangulo de '=' vira um tapete so
 *   M   mesa (movel: bloqueia o movimento)
 *   C   computador em cima de uma mesa (movel: bloqueia o movimento)
 *   P   onde o jogador comeca no andar
 *   S   escada para o proximo andar (saida)
 *   I   bateria (item coletavel)
 *   F   professor do andar (quem libera a escada depois do quiz)
 *   R X Z B robos (reservados para a parte dos robos; por enquanto viram piso)
 * As letras de objetos (P, S, I, F, R, X, Z, L) viram piso na matriz: o tipo
 * de piso (corredor ou sala) e copiado da casa a esquerda.
 *
 * Os andares se ligam: o S de um andar fica na mesma posicao do P do proximo,
 * como se o jogador tivesse acabado de subir a escada.
 *   andar 1 (terreo):        P (1,1)   -> S (22,38)   salas 101/102, auditorio, secretaria, cantina, recepcao
 *   andar 2 (laboratorios):  P (22,38) -> S (1,1)     labs 1 a 3, salas 201/202, sala dos professores
 *   andar 3 (biblioteca):    P (1,1)   -> S (12,19)   biblioteca, servidores no centro, coordenacao, salas 301/302
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

// Tapetes: no arquivo do andar, um retangulo de '=' vira um tapete laranja
// com a logo do CESAR no meio (andavel, como o piso).
bool mapaTemTapete(struct Mapa *mapa, int linha, int coluna);
// true se a casa e o canto de cima/esquerda de um tapete; devolve o tamanho em casas
bool mapaCantoTapete(struct Mapa *mapa, int linha, int coluna, int *largura, int *altura);

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
