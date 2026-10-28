#include <stdio.h>
#include "mapa.h"
#include "memoria.h"

#define TAM_BUFFER 256

int mapaCarregar(Jogo *jogo, const char *arquivo) {
    if (jogo == NULL || arquivo == NULL) return 0;

    FILE *f = fopen(arquivo, "r");
    if (f == NULL) {
        printf("Erro: nao achei o arquivo %s\n", arquivo);
        return 0;
    }

    int linhas, colunas;
    if (fscanf(f, "%d %d", &linhas, &colunas) != 2 || linhas <= 0 || colunas <= 0 || colunas > TAM_BUFFER - 2) {
        printf("Erro: cabecalho invalido em %s\n", arquivo);
        fclose(f);
        return 0;
    }

    char buffer[TAM_BUFFER];
    fgets(buffer, sizeof(buffer), f);

    Mapa *mapa = &jogo->mapa;
    mapa->linhas = linhas;
    mapa->colunas = colunas;
    mapa->matriz = memAlocar(linhas * sizeof(char *));
    if (mapa->matriz == NULL) {
        fclose(f);
        return 0;
    }

    for (int i = 0; i < linhas; i++) {
        mapa->matriz[i] = memAlocar(colunas * sizeof(char));
        if (mapa->matriz[i] == NULL) {
            mapa->linhas = i;
            mapaLiberar(mapa);
            fclose(f);
            return 0;
        }

        if (fgets(buffer, sizeof(buffer), f) == NULL) buffer[0] = '\0';

        int fimLinha = 0;
        for (int j = 0; j < colunas; j++) {
            char c = fimLinha ? ' ' : buffer[j];

            if (c == '\n' || c == '\r' || c == '\0') {
                fimLinha = 1;
                c = ' ';
            }

            switch (c) {
                case 'P':
                    jogo->jogador.posicaoInicial.linha = i;
                    jogo->jogador.posicaoInicial.coluna = j;
                    jogo->jogador.posicao = jogo->jogador.posicaoInicial;
                    mapa->matriz[i][j] = ' ';
                    break;
                case '.':
                case 'L':
                case 'R':
                case 'S':
                case 'I':
                    mapa->matriz[i][j] = ' ';
                    break;
                default:
                    mapa->matriz[i][j] = c;
                    break;
            }
        }
    }

    fclose(f);
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
