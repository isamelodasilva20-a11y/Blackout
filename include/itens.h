#ifndef ITENS_H
#define ITENS_H

#include "raylib.h"
#include "estrutura.h"

void adicionar_item(Item **item, Posicao posicao, TipoItem tipo);

void coletar_item(Item **item, Jogador *jogador);

void liberar_item(Item *item);

void liberar_todos_itens(Item **itens);

void desenhar_item(struct Jogo *jogo, Vector2 origem, int tamanhoCasa);

void desenhar_item_3d(struct Jogo *jogo, Camera3D camera);

int verificar_vitoria(Item *itens);

int contar_itens(Item *itens);

#endif