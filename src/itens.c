#include <stdlib.h>
#include <stddef.h>
#include "raylib.h"
#include "jogo.h"       // 1. jogo.h define Posicao, TipoItem, Item
#include "memoria.h"    // 2. OBRIGATÓRIO: Traz as funções memAlocar e memLiberar
#include "itens.h"      // 3. Protótipos dos itens

//aloca a memória dos itens necessários no inicio do jogo
void adicionar_item(Item **itens, Posicao posicao, TipoItem tipo){
    Item *novo = (Item *)memAlocar(sizeof(Item));
    novo->posicao = posicao;
    novo->tipo = tipo;
    novo->next = *itens;
    *itens = novo;
}

//desenha a representação visual dos itens
//origem = canto superior esquerdo do mapa na tela (no minimapa ela muda)
void desenhar_item(Item *itens, Vector2 origem, int tamanhoCasa){
    Item *atual = itens;
    while(atual!=NULL){
        //pega a loc atribuida ao item para desenhar no local certo
        int eixo_x = (int)origem.x + (atual->posicao.coluna * tamanhoCasa) + (tamanhoCasa / 2);
        int eixo_y = (int)origem.y + (atual->posicao.linha * tamanhoCasa) + (tamanhoCasa / 2);

        //usa a funcao do raylib pra desenhar, inicialmente um cículo azul
        DrawCircle(eixo_x, eixo_y, (float)tamanhoCasa / 4.0f, BLUE);
        //vai pro proximo item até chegar em NULL
        atual = atual->next;
    }
}

//verificar a coleta do item pelo jogador
void coletar_item(Item **itens, Jogador *jogador){
    //verifica se existem os itens q serão analisados
    if (itens == NULL || *itens == NULL || jogador == NULL) return;
    //auxiliares locais
    Item *atual = *itens;
    Item *anterior = NULL;
    while(atual != NULL){
        //verifica se a posição do jogador é igual à do item
        if(jogador->posicao.linha == atual->posicao.linha && jogador->posicao.coluna == atual->posicao.coluna){
            jogador->bateria = 100;//recarrega a bateria
            Item *apagado = atual;//atribui o item que será apagado apos a coleta
            atual = atual->next;//atual se liga ao proximo item
            if(anterior==NULL){
                *itens = atual;//significa que é o primeiro número
            }else{
                anterior->next = atual;//fica logo atrás
            }

            liberar_item(apagado);//apaga o item coletado
            break;

        }else{
            //anda na memória
            anterior = atual;
            atual = atual->next;
        }
    }
}

int verificar_vitoria(Item *itens){
    //condicional implicita que devolve 0(f) ou 1(v)
    //ganha quando os itens acabam
    return(itens==NULL);
}

int contar_itens(Item *itens){
    //usa um for para percorrer a lista de itens, usnaod um contador pra saber quantos faltam.
    int total = 0;
    for(Item *atual = itens; atual != NULL; atual = atual->next){
        total++;
    }
    return total;
}

void liberar_item(Item *itens){
    //recebe um item e chama a funcao de liberar memória
    if(itens!=NULL){
        memLiberar(itens);
    }
}

void liberar_todos_itens(Item **itens){
    //libera todos os itens de uma vez, caso encerre o jogo no meio da partida
    if(itens == NULL || *itens == NULL)return;
    Item *atual = *itens;
    while(atual!=NULL){
        Item *proximo = atual->next;
        liberar_item(atual);
        atual = proximo;
    }
    *itens = NULL;
}
