#ifndef MAPA_H
#define MAPA_H

#include "jogo.h"

/*
 * Formato dos arquivos andares/andarN.txt
 *
 * Primeira linha: "<linhas> <colunas>" (hoje todos os andares sao 25 40,
 * que com casas de 24px ocupam 960x600 da janela de 1024x640).
 * Depois vem o desenho do mapa, um caractere por casa:
 *
 *   #   parede (unica casa que bloqueia o movimento)
 *   .   corredor
 *   ' ' (espaco) chao de sala - tambem e andavel
 *   P   onde o jogador comeca no andar
 *   S   escada para o proximo andar (saida)
 *   I   bateria (item que precisa ser coletado para liberar a escada)
 *   R   robo (ainda nao e lido - parte dos robos)
 *   L   lanterna (ignorada, a lanterna agora e fixa)
 *
 * Os andares se ligam: o S de um andar fica na mesma posicao do P do proximo,
 * como se o jogador tivesse acabado de subir a escada.
 *   andar 1: P (1,1)   -> S (22,38)   2 baterias
 *   andar 2: P (22,38) -> S (1,1)     3 baterias  (laboratorios)
 *   andar 3: P (1,1)   -> S (12,19)   4 baterias  (sala de servidores no centro)
 */

#define TOTAL_ANDARES 3 // quantos arquivos andarN.txt o jogo percorre ate a vitoria

void desenharMapa2D(Mapa *mapa, int tamanhoCasa);

// liberada = true quando todas as baterias ja foram coletadas (escada verde)
void desenharSaida2D(Mapa *mapa, int tamanhoCasa, bool liberada);

// Le um arquivo de mapa para dentro de jogo->mapa. Tambem posiciona o jogador
// (P), guarda a escada (S) e cria as baterias (I) na lista jogo->itens.
// Retorna 1 se deu certo e 0 se deu erro.
int  mapaCarregar(Jogo *jogo, const char *arquivo);

// Troca para o andar indicado (1 a TOTAL_ANDARES): libera o mapa e os itens
// do andar atual e carrega andares/andarN.txt. Retorna 1 se deu certo.
int  mapaCarregarAndar(Jogo *jogo, int andar);

// Devolve para o sistema toda a memoria da matriz do mapa.
void mapaLiberar(Mapa *mapa);

#endif
