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
void desenhar_item(Jogo *jogo, Vector2 origem, int tamanhoCasa) {
    if (jogo == NULL) return;

    Item *atual = jogo->itens;
    while (atual != NULL) {
        // Converte a posição do item no mapa para pixels na tela
        int posX = (int)origem.x + (atual->posicao.coluna * tamanhoCasa);
        int posY = (int)origem.y + (atual->posicao.linha * tamanhoCasa);

        if (atual->tipo == ITEM_BATERIA && jogo->texturaItem.id > 0) {
            // Recorte da textura original da bateria
            Rectangle src = { 0.0f, 0.0f, (float)jogo->texturaItem.width, (float)jogo->texturaItem.height };
            // Destino na tela (ajustado exatamente para o tamanho do quadrado da casa)
            Rectangle dest = { (float)posX, (float)posY, (float)tamanhoCasa*2, (float)tamanhoCasa };
            
            DrawTexturePro(jogo->texturaItem, src, dest, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
        } else {
            // Fallback em círculo caso a imagem falhe
            int centroX = posX + (tamanhoCasa / 2);
            int centroY = posY + (tamanhoCasa / 2);
            DrawCircle(centroX, centroY, (float)tamanhoCasa / 4.0f, BLUE);
        }

        atual = atual->next;
    }
}

#include <math.h>

void desenhar_item_3d(Jogo *jogo, Camera3D camera) {
    if (jogo == NULL || jogo->itens == NULL) return;

    float tempo = (float)GetTime();
    Item *atual = jogo->itens;

    while (atual != NULL) {
        if (atual->tipo == ITEM_BATERIA && jogo->texturaItem.id > 0) {
            float cx = (float)atual->posicao.coluna + 0.5f;
            float cz = (float)atual->posicao.linha + 0.5f;

            // Altura onde a pilha flutua no 3D
            float alturaY = 0.30f + 0.05f * sinf(tempo * 3.0f + cx);
            Vector3 posicaoItem = { cx, alturaY, cz };

            Rectangle recTextura = { 0.0f, 0.0f, (float)jogo->texturaItem.width, (float)jogo->texturaItem.height };

            // Desenha a imagem 2D da bateria virada para a câmera 3D
            DrawBillboardRec(camera, jogo->texturaItem, recTextura, posicaoItem, (Vector2){ 1.0f, 0.5f }, WHITE);
        }

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
