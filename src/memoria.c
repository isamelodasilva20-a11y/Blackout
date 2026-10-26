#include <stdio.h>
#include "memoria.h"

static int blocos = 0;

void *memAlocar(size_t tamanho) {
    void *p = malloc(tamanho);
    if (p != NULL) {
        blocos++;
    }
    return p;
}

void memLiberar(void *ptr) {
    if (ptr != NULL) {
        free(ptr);
        blocos--;
    }
}

void memRelatorio(void) {
    printf("Blocos nao liberados: %d\n", blocos);
}
