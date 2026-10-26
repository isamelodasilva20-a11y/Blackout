#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdlib.h>

void *memAlocar(size_t tamanho);
void  memLiberar(void *ptr);
void  memRelatorio(void);

#endif