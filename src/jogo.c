#include <stddef.h>
#include "jogo.h"
#include "robos.h"

void jogoTick(Jogo *jogo) {
    if (jogo == NULL) return;

    static int contadorTicks = 0;
    contadorTicks++;

    // A cada 4 ticks (200 ms), atualiza a posição dos robôs
    if (contadorTicks >= 4) {
        robosAtualizar(jogo);
        contadorTicks = 0;
    }

    // Checa colisões a cada 50 ms
    checarColisoes(jogo);
}