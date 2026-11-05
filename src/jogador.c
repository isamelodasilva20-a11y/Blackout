#include <stdio.h>
#include <stddef.h>
#include "raylib.h"
#include "jogo.h"       // 1. Traz as definições completas de Direcao, Jogador e Mapa
#include "mapa.h"       // 2. OBRIGATÓRIO: Traz a função mapaCasaBloqueada
#include "jogador.h"    // 3. Protótipos do jogador

// O sprite e desenhado 1,6x maior que a casa (24px -> ~38px)
#define ESCALA_SPRITE 1.6f

//responsável pelos movimentos do jogador
void movimentar_jogador(Jogador *jogador, Mapa *mapa, Direcao direcao){
    if(jogador == NULL || mapa == NULL || mapa->matriz == NULL) return;

    //assume a posiçao atual, a proxima virá a partir do next dessa
    int nova_linha = jogador->posicao.linha;
    int nova_coluna = jogador->posicao.coluna;

    //define em qual direcão o jogador está indo
    jogador->direcao = direcao;

    //switch case é um tipo mais simples de condicional
    switch (direcao){
        //anda pela matriz de acordo com a direção
        case DIR_CIMA: nova_linha--; break;
        case DIR_BAIXO: nova_linha++; break;
        case DIR_DIREITA: nova_coluna++; break;
        case DIR_ESQUERDA: nova_coluna--; break;
    }
    //verifica se a casa de destino está livre: dentro do mapa e sem parede ou móvel
    //(a regra fica no mapa.c, no mapaCasaBloqueada)
    if(!mapaCasaBloqueada(mapa, nova_linha, nova_coluna)){
        jogador->posicao.linha = nova_linha;
        jogador->posicao.coluna = nova_coluna;

        // ALTERNA O PASSO: se estava no frame 1 vai pro 2, se estava no 2 (ou 0) vai pro 1
        if (jogador->frame == 1) {
            jogador->frame = 2;
        } else {
            jogador->frame = 1;
        }
    }
}

//gira 90 graus no lugar (visão 3D: A/D)
//sentido horário visto de cima: cima -> direita -> baixo -> esquerda -> cima
void girar_jogador(Jogador *jogador, bool paraDireita){
    if(jogador == NULL) return;

    switch (jogador->direcao){
        case DIR_CIMA:     jogador->direcao = paraDireita ? DIR_DIREITA  : DIR_ESQUERDA; break;
        case DIR_DIREITA:  jogador->direcao = paraDireita ? DIR_BAIXO    : DIR_CIMA;     break;
        case DIR_BAIXO:    jogador->direcao = paraDireita ? DIR_ESQUERDA : DIR_DIREITA;  break;
        case DIR_ESQUERDA: jogador->direcao = paraDireita ? DIR_CIMA     : DIR_BAIXO;    break;
    }
}

//anda uma casa para trás (visão 3D: S) sem virar de costas
void recuar_jogador(Jogador *jogador, Mapa *mapa){
    if(jogador == NULL) return;

    //guarda para onde o jogador está olhando
    Direcao frente = jogador->direcao;

    //a direção oposta à da frente
    Direcao tras = DIR_BAIXO;
    switch (frente){
        case DIR_CIMA:     tras = DIR_BAIXO;    break;
        case DIR_BAIXO:    tras = DIR_CIMA;     break;
        case DIR_ESQUERDA: tras = DIR_DIREITA;  break;
        case DIR_DIREITA:  tras = DIR_ESQUERDA; break;
    }

    //reaproveita o movimento normal (que também testa paredes e móveis)
    //e depois volta a olhar para a frente
    movimentar_jogador(jogador, mapa, tras);
    jogador->direcao = frente;
}

void desenharJogador2D(Jogo *jogo, Vector2 origem, int tamanhoCasa) {
    if (jogo == NULL || jogo->texturaJogador.id == 0) return;

    // Tamanho do boneco na tela: 1.6x o tamanho da casa
    float tamanho = (float)tamanhoCasa * ESCALA_SPRITE;

    // Calcula o CENTRO base da casa no mapa
    float posX = origem.x + (float)(jogo->jogador.posicao.coluna * tamanhoCasa) + ((float)tamanhoCasa / 2.0f);
    float posY = origem.y + (float)(jogo->jogador.posicao.linha * tamanhoCasa + tamanhoCasa) - tamanho;

    // Dimensões do recorte na folha de sprites (4 colunas x 3 linhas)
    float larguraFrame = (float)jogo->texturaJogador.width / 4.0f;
    float alturaFrame  = (float)jogo->texturaJogador.height / 3.0f;

    int linhaDirecao = 0;

    if (jogo->jogador.direcao == DIR_CIMA) {
        linhaDirecao = 0;
    } else if (jogo->jogador.direcao == DIR_BAIXO) {
        linhaDirecao = 1;
    } else if (jogo->jogador.direcao == DIR_ESQUERDA || jogo->jogador.direcao == DIR_DIREITA) {
        linhaDirecao = 2; // Perfil
    }

    // PADDING: Margem de 2px
    float padding = 2.0f;
    float xInicial = (float)(jogo->jogador.frame % 4) * larguraFrame + padding;
    float larguraSrc = larguraFrame - (padding * 2.0f);

    // Se estiver a olhar para a DIREITA, inverte a largura para espelhar
    if (jogo->jogador.direcao == DIR_DIREITA) {
        larguraSrc = -larguraSrc;
    }

    Rectangle srcRec = {
        .x = xInicial,
        .y = (float)linhaDirecao * alturaFrame,
        .width = larguraSrc,
        .height = alturaFrame
    };

    // SOMBRA: Mantida perfeitamente centralizada na casa
    DrawEllipse((int)posX, (int)(posY + tamanho - 3),
                (float)tamanhoCasa * 0.35f, (float)tamanhoCasa * 0.15f, Fade(BLACK, 0.4f));

    // AJUSTE DE CENTRALIZAÇÃO DO CORPO POR DIREÇÃO:
    float offsetX = 0.0f;

    if (jogo->jogador.direcao == DIR_BAIXO || jogo->jogador.direcao == DIR_CIMA) {
        // De frente ou de costas: a lanterna fica à direita, empurrando o corpo para a esquerda.
        // Empurramos o sprite ligeiramente para a DIREITA (~10% do tamanho) para alinhar os pés na sombra.
        offsetX = tamanho * 0.10f;
    } else if (jogo->jogador.direcao == DIR_ESQUERDA) {
        // Olhando para a ESQUERDA: empurra para a direita
        offsetX = tamanho * 0.15f;
    } else if (jogo->jogador.direcao == DIR_DIREITA) {
        // Olhando para a DIREITA: empurra para a esquerda
        offsetX = -tamanho * 0.15f;
    }

    // Posiciona o retângulo de destino com a compensação no eixo X
    Rectangle destRec = { posX + offsetX, posY, tamanho, tamanho };
    
    // Pivô no centro horizontal do sprite
    Vector2 origin = { tamanho / 2.0f, 0.0f };

    DrawTexturePro(jogo->texturaJogador, srcRec, destRec, origin, 0.0f, WHITE);
}


//seta amarela apontando para a direção do jogador (minimapa do modo 3D,
//onde o sprite ficaria pequeno demais para enxergar)
void desenharSetaJogador2D(Jogador *jogador, Vector2 origem, int tamanhoCasa) {
    if (jogador == NULL) return;

    //centro da casa do jogador, em pixels
    Vector2 centro = {
        origem.x + (float)(jogador->posicao.coluna * tamanhoCasa) + (float)tamanhoCasa / 2.0f,
        origem.y + (float)(jogador->posicao.linha * tamanhoCasa) + (float)tamanhoCasa / 2.0f
    };

    //frente = vetor de tamanho 1 para onde o jogador olha (y cresce para baixo na tela)
    Vector2 frente = { 0.0f, 1.0f };
    switch (jogador->direcao) {
        case DIR_CIMA:     frente = (Vector2){  0.0f, -1.0f }; break;
        case DIR_BAIXO:    frente = (Vector2){  0.0f,  1.0f }; break;
        case DIR_ESQUERDA: frente = (Vector2){ -1.0f,  0.0f }; break;
        case DIR_DIREITA:  frente = (Vector2){  1.0f,  0.0f }; break;
    }
    //lado = frente girada 90 graus
    Vector2 lado = { -frente.y, frente.x };
    float r = (float)tamanhoCasa * 1.1f; //a seta é um pouco maior que a casa para aparecer bem

    //ponta na frente e as duas pontas de trás para os lados
    //(a raylib pede os vértices em sentido anti-horário)
    Vector2 ponta    = { centro.x + frente.x * r, centro.y + frente.y * r };
    Vector2 trasEsq  = { centro.x - frente.x * r * 0.7f - lado.x * r * 0.7f, centro.y - frente.y * r * 0.7f - lado.y * r * 0.7f };
    Vector2 trasDir  = { centro.x - frente.x * r * 0.7f + lado.x * r * 0.7f, centro.y - frente.y * r * 0.7f + lado.y * r * 0.7f };
    DrawTriangle(ponta, trasEsq, trasDir, YELLOW);
}
