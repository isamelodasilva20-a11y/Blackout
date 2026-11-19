#ifndef CORES_H
#define CORES_H

#include "raylib.h"

// Paleta do guia de marca do CESAR, usada nas telas (inicial, HUD, pausa,
// game over e vitoria). Fica num lugar so para as telas nao terem copias
// diferentes das mesmas cores.
#define COR_FUNDO    (Color){  22,  22,  22, 255 } // #161616
#define COR_LARANJA  (Color){ 255,  96,   2, 255 } // #ff6002
#define COR_LARANJA2 (Color){ 221,  64,  18, 255 } // #dd4012
#define COR_CLARO    (Color){ 255, 254, 238, 255 } // #fffeee
#define COR_CINZA    (Color){ 129, 139, 157, 255 } // #818b9d

#endif
