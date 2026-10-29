#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "jogo.h"
#include "itens.h"
#include "memoria.h"

void adicionar_item(Item **itens, Posicao posicao, TipoItem tipo){
    Item *novo = (Item *)memAlocar(sizeof(Item));
    novo->posicao = posicao;
    novo->tipo = tipo;
    novo->next = *itens;
    *itens = novo;
}

void desenhar_item(Item *itens, int tamanhoCasa){
    Item *atual = itens;
    while(atual!=NULL){
        int eixo_x = (atual->posicao.coluna * tamanhoCasa) + (tamanhoCasa / 2.0f);
        int eixo_y = (atual->posicao.linha * tamanhoCasa) + (tamanhoCasa / 2.0f);

        if(atual->tipo == ITEM_PENDRIVE){
            DrawCircle((int)eixo_x, (int)eixo_y, tamanhoCasa / 4.0f, BLUE);
        }else if(atual->tipo == ITEM_LANTERNA){
            DrawCircle((int)eixo_x, (int)eixo_y, tamanhoCasa / 4.0f, GREEN);
        }
        atual = atual->next;
    }
}

void coletar_item(Item **itens, Jogador *jogador){
    if (itens == NULL || *itens == NULL || jogador == NULL) return;
    Item *atual = *itens;
    Item *anterior = NULL;
    while(atual != NULL){
        if(jogador->posicao.linha == atual->posicao.linha && jogador->posicao.coluna == atual->posicao.coluna){
            if(atual->tipo == ITEM_PENDRIVE){
                jogador->pontos += 10;
            }else if(atual->tipo == ITEM_LANTERNA){
                jogador->bateria = 100;
            }
            Item *apagado = atual;
            atual = atual->next;
            if(anterior==NULL){
                *itens = atual;
            }else{
                anterior->next = atual;
            }

            liberar_item(apagado);
            break;

        }else{
            anterior = atual;
            atual = atual->next;
        }
    }
}

int verificar_vitoria(Item *itens){
    return(itens==NULL);
}

void liberar_item(Item *itens){
    if(itens!=NULL){
        memLiberar(itens);
    }
}

void liberar_todos_itens(Item **itens){
    if(itens == NULL || *itens == NULL)return;
    Item *atual = *itens;
    while(atual!=NULL){
        Item *proximo = atual->next;
        liberar_item(atual);
        atual = proximo;
    }
    *itens = NULL;
}
