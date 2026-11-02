#include <stdio.h>
#include "mapa.h"
#include "memoria.h"
#include "itens.h"

#define TAM_BUFFER 256 // tamanho maximo de uma linha lida do arquivo

#include <stddef.h>
#include "raylib.h"
#include "jogo.h"

void desenharMapa2D(Mapa *mapa, int tamanhoCasa) {
    if(mapa == NULL || mapa->matriz == NULL) return;

    for(int i = 0; i < mapa->linhas; i++){
        for(int j = 0; j < mapa->colunas; j++){
            if(mapa->matriz[i][j] == '#'){
                DrawRectangle(j * tamanhoCasa, i * tamanhoCasa, tamanhoCasa, tamanhoCasa, DARKBLUE);
            }
        }
    }
}

// Desenha a escada (saida do andar) como um quadrado na casa do 'S'
void desenharSaida2D(Mapa *mapa, int tamanhoCasa, bool liberada) {
    if(mapa == NULL || mapa->saida.linha < 0) return; // andar sem escada

    // Escada fica cinza enquanto ainda faltam baterias e verde quando o jogador pode subir
    Color cor = liberada ? GREEN : DARKGRAY;

    // Converte linha/coluna em pixels; o +2 / -4 deixa uma borda de 2px para o
    // quadrado nao encostar nas paredes vizinhas
    DrawRectangle(mapa->saida.coluna * tamanhoCasa + 2, mapa->saida.linha * tamanhoCasa + 2,
                  tamanhoCasa - 4, tamanhoCasa - 4, cor);
}


// A legenda dos caracteres do mapa esta em mapa.h
int mapaCarregar(Jogo *jogo, const char *arquivo) {
    if (jogo == NULL || arquivo == NULL) return 0;

    FILE *f = fopen(arquivo, "r");
    if (f == NULL) {
        printf("Erro: nao achei o arquivo %s\n", arquivo);
        return 0;
    }

    // 1) Cabecalho: tamanho do mapa ("25 40")
    int linhas, colunas;
    if (fscanf(f, "%d %d", &linhas, &colunas) != 2 || linhas <= 0 || colunas <= 0 || colunas > TAM_BUFFER - 2) {
        printf("Erro: cabecalho invalido em %s\n", arquivo);
        fclose(f);
        return 0;
    }

    // Descarta o resto da linha do cabecalho (a quebra de linha)
    char buffer[TAM_BUFFER];
    fgets(buffer, sizeof(buffer), f);

    // 2) Aloca a matriz: um vetor de ponteiros, um para cada linha do mapa
    Mapa *mapa = &jogo->mapa;
    mapa->linhas = linhas;
    mapa->colunas = colunas;
    mapa->saida = (Posicao){ .linha = -1, .coluna = -1 }; // -1 = andar sem escada, ate achar um 'S'
    mapa->matriz = memAlocar(linhas * sizeof(char *));
    if (mapa->matriz == NULL) {
        fclose(f);
        return 0;
    }

    // 3) Le o mapa linha por linha, casa por casa
    for (int i = 0; i < linhas; i++) {
        mapa->matriz[i] = memAlocar(colunas * sizeof(char));
        if (mapa->matriz[i] == NULL) {
            mapa->linhas = i; // so libera as linhas que chegaram a ser alocadas
            mapaLiberar(mapa);
            fclose(f);
            return 0;
        }

        if (fgets(buffer, sizeof(buffer), f) == NULL) buffer[0] = '\0';

        // Se a linha do arquivo for mais curta que o mapa (ou acabar em \r\n,
        // como no Windows), o que falta vira chao vazio
        int fimLinha = 0;
        for (int j = 0; j < colunas; j++) {
            char c = fimLinha ? ' ' : buffer[j];

            if (c == '\n' || c == '\r' || c == '\0') {
                fimLinha = 1;
                c = ' ';
            }

            // Letras especiais sao lidas aqui e viram chao (' ') na matriz:
            // a matriz guarda so o cenario, e os objetos ficam nas suas proprias structs
            switch (c) {
                case 'P':
                    // Ponto de partida do jogador neste andar
                    jogo->jogador.posicaoInicial.linha = i;
                    jogo->jogador.posicaoInicial.coluna = j;
                    jogo->jogador.posicao = jogo->jogador.posicaoInicial;
                    mapa->matriz[i][j] = ' ';
                    break;
                case 'S':
                    // Escada: guardamos a posicao para o main saber quando o jogador sobe
                    mapa->saida.linha = i;
                    mapa->saida.coluna = j;
                    mapa->matriz[i][j] = ' ';
                    break;
                case 'I':
                    // Bateria: entra na lista encadeada de itens do jogo
                    adicionar_item(&jogo->itens, (Posicao){ .linha = i, .coluna = j }, ITEM_BATERIA);
                    mapa->matriz[i][j] = ' ';
                    break;
                case '.':
                case 'L': // lanterna agora e fixa, nao vira item
                case 'R': // robos: ainda serao lidos aqui pela parte dos robos
                    mapa->matriz[i][j] = ' ';
                    break;
                default:
                    // '#' (parede) e ' ' (chao de sala) ficam como estao
                    mapa->matriz[i][j] = c;
                    break;
            }
        }
    }

    fclose(f);
    return 1;
}

// Troca de andar: descarta o mapa e os itens atuais e carrega andares/andarN.txt
// Usada tanto no inicio do jogo (andar 1) quanto quando o jogador sobe a escada.
int mapaCarregarAndar(Jogo *jogo, int andar) {
    if (jogo == NULL || andar < 1 || andar > TOTAL_ANDARES) return 0;

    // Limpa o andar anterior antes de carregar o novo, para nao vazar memoria.
    // No primeiro andar a lista e a matriz ainda estao vazias (NULL), e as
    // duas funcoes simplesmente nao fazem nada.
    liberar_todos_itens(&jogo->itens);
    mapaLiberar(&jogo->mapa);

    // Monta o nome do arquivo a partir do numero do andar: 2 -> "andares/andar2.txt"
    char arquivo[64];
    snprintf(arquivo, sizeof(arquivo), "andares/andar%d.txt", andar);
    if (!mapaCarregar(jogo, arquivo)) return 0;

    // Vidas, pontos e bateria do jogador nao sao tocados: passam de um andar para o outro.
    // So a posicao muda, porque o mapaCarregar coloca o jogador no 'P' do novo andar.
    jogo->andarAtual = andar;
    return 1;
}

void mapaLiberar(Mapa *mapa) {
    if (mapa == NULL || mapa->matriz == NULL) return;

    for (int i = 0; i < mapa->linhas; i++) {
        memLiberar(mapa->matriz[i]);
    }
    memLiberar(mapa->matriz);

    mapa->matriz = NULL;
    mapa->linhas = 0;
    mapa->colunas = 0;
}
