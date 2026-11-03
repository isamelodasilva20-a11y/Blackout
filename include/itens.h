#ifndef ITENS_H
#define ITENS_H
#include "jogo.h"

void adicionar_item(Item **item, Posicao posicao, TipoItem tipo);

void coletar_item(Item **item, Jogador *jogador);

void liberar_item(Item *item);

void liberar_todos_itens(Item **itens);

// origem = canto superior esquerdo do mapa na tela (permite desenhar no minimapa)
void desenhar_item(Item *item, Vector2 origem, int tamanhoCasa);

int verificar_vitoria(Item *itens);

// Quantos itens ainda estao no mapa
int contar_itens(Item *itens);
#endif