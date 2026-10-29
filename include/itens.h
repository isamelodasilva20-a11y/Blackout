#ifndef ITENS_H
#define ITENS_H

#include "jogo.h"

void adicionar_item(Item **item, Posicao posicao, TipoItem tipo);

void coletar_item(Item **item, Jogador *jogador);

void liberar_item(Item *item);

void liberar_todos_itens(Item **itens);

void desenhar_item(Item *item, int tamanhoCasa);

int verificar_vitoria(Item *itens);
#endif