#include <math.h>
#include <stddef.h>
#include "raylib.h"
#include "rlgl.h"   // camada de baixo nivel que vem junto com a raylib
#include "render3d.h"
#include "mapa.h"
#include "itens.h"
#include "professor.h"
#include "jogador.h"
#include "robos.h"
#include "hud.h"

// Medidas do mundo 3D (1.0 = tamanho de uma casa do mapa)
#define ALTURA_PAREDE   1.0f
#define ALTURA_OLHOS    0.5f
#define CAMPO_VISAO     70.0f  // abertura da camera, em graus
#define RAIO_VISAO      7.0f   // ate quantas casas de distancia ainda se enxerga algo
#define TEMPO_TRANSICAO 0.15f  // duracao da animacao de andar/girar, em segundos

// Minimapa (medidas no tamanho base da janela; sao multiplicadas pela escala)
#define CASA_MINIMAPA   6
#define MARGEM_MINIMAPA 10

// Cores do mundo 3D (antes de escurecer pela distancia): paleta do guia de
// marca do CESAR, igual a do mapa 2D
#define COR_LARANJA_3D   (Color){ 255,  96,   2, 255 } // #ff6002
#define COR_CLARO_3D     (Color){ 255, 254, 238, 255 } // #fffeee
#define COR_PAREDE_3D    (Color){ 129, 139, 157, 255 } // #818b9d
#define COR_RODAPE_3D    COR_LARANJA_3D                // faixa laranja ao pe da parede
#define COR_CORREDOR_3D  (Color){  48,  48,  48, 255 }
#define COR_PLACA_3D     (Color){  63,  63,  63, 255 } // #3f3f3f: placa no meio do corredor
#define COR_SALA_A_3D    COR_CLARO_3D
#define COR_SALA_B_3D    (Color){ 232, 230, 210, 255 }
#define COR_FRISO_3D     (Color){  63,  63,  63, 255 } // #3f3f3f: friso no alto da parede
#define COR_TETO_3D      (Color){  22,  22,  22, 255 } // #161616
#define COR_MADEIRA_3D   (Color){  63,  63,  63, 255 } // mesas em grafite
#define COR_MADEIRA_E_3D (Color){  22,  22,  22, 255 }
#define COR_METAL_3D     (Color){  63,  63,  63, 255 }
#define COR_METAL_E_3D   (Color){  22,  22,  22, 255 }
#define COR_TELA_3D      COR_LARANJA_3D                // telas acesas em laranja

// ---- Camera ----------------------------------------------------------------

// Uma "pose" da camera: onde ela esta no chao (x, z) e para onde olha (angulo)
typedef struct {
    float x, z, angulo;
} Pose;

// A logica pula de casa em casa, mas a camera "desliza" da pose "de" ate a
// pose "para"; progresso vai de 0 (comecando) ate 1 (terminou).
typedef struct {
    Pose atual, de, para;
    float progresso;
    Posicao posicaoAlvo;  // ultima casa e direcao do jogador que a camera viu
    Direcao direcaoAlvo;
} CameraJogo;

// Comeca zerada, na casa (0,0), que e sempre parede: na primeira atualizacao
// o jogador esta longe, e a regra do "pulo grande" ja coloca a camera nele.
static CameraJogo cam = { 0 };

// Angulo (em radianos) de cada direcao no plano x/z. 0 = olhando para +x
// (direita); como z = linha, "baixo" no mapa e +z.
static const float ANGULO[] = { [DIR_DIREITA] = 0.0f, [DIR_BAIXO] = PI / 2.0f,
                                [DIR_ESQUERDA] = PI,  [DIR_CIMA]  = -PI / 2.0f };

// Meio da casa n no mundo 3D (serve para linha -> z e para coluna -> x)
static float centro(int n) {
    return (float)n + 0.5f;
}

// Deixa um giro entre -180 e +180 graus (em radianos) para a camera girar pelo
// caminho mais curto. Ex.: de -90 (cima) para 180 (esquerda) daria 270 graus
// (3/4 de volta); ajustado vira -90, so 1/4 de volta para o outro lado.
static float giroMaisCurto(float giro) {
    while (giro >  PI) giro -= 2.0f * PI;
    while (giro < -PI) giro += 2.0f * PI;
    return giro;
}

// Pose da camera parada na casa do jogador, olhando para a direcao dele
static Pose poseDoJogador(Jogador *jogador) {
    return (Pose){ centro(jogador->posicao.coluna), centro(jogador->posicao.linha), ANGULO[jogador->direcao] };
}

// Pula a camera direto para a casa e a direcao do jogador (sem animacao):
// usada ao ligar o 3D e quando o jogador "teleporta" (troca de andar)
void render3dReiniciarCamera(Jogo *jogo) {
    if (jogo == NULL) return;

    cam.atual = poseDoJogador(&jogo->jogador);
    cam.progresso = 1.0f; // nenhuma animacao em andamento
    cam.posicaoAlvo = jogo->jogador.posicao;
    cam.direcaoAlvo = jogo->jogador.direcao;
}

// Chamada todo quadro: percebe quando o jogador mudou de casa ou de direcao
// e faz a camera deslizar ate la em TEMPO_TRANSICAO segundos
void render3dAtualizarCamera(Jogo *jogo, float dt) {
    if (jogo == NULL) return;

    Posicao p = jogo->jogador.posicao;
    Direcao d = jogo->jogador.direcao;

    // O jogador mudou: comeca uma nova animacao a partir de onde a camera esta
    if (p.linha != cam.posicaoAlvo.linha || p.coluna != cam.posicaoAlvo.coluna || d != cam.direcaoAlvo) {
        Pose alvo = poseDoJogador(&jogo->jogador);

        // Pulo de mais de uma casa (ex.: troca de andar): vai direto, sem animar
        if (fabsf(alvo.x - cam.atual.x) + fabsf(alvo.z - cam.atual.z) > 1.5f) {
            render3dReiniciarCamera(jogo);
            return;
        }

        alvo.angulo = cam.atual.angulo + giroMaisCurto(alvo.angulo - cam.atual.angulo);
        cam.de = cam.atual;
        cam.para = alvo;
        cam.progresso = 0.0f;
        cam.posicaoAlvo = p;
        cam.direcaoAlvo = d;
    }

    if (cam.progresso >= 1.0f) return; // parada: nada para animar

    cam.progresso = fminf(cam.progresso + dt / TEMPO_TRANSICAO, 1.0f);

    // Curva "smoothstep" (comeca devagar, acelera e freia no final) e, com ela,
    // o ponto entre "de" e "para": de + (para - de) * s
    float t = cam.progresso, s = t * t * (3.0f - 2.0f * t);
    cam.atual.x      = cam.de.x + (cam.para.x - cam.de.x) * s;
    cam.atual.z      = cam.de.z + (cam.para.z - cam.de.z) * s;
    cam.atual.angulo = cam.de.angulo + (cam.para.angulo - cam.de.angulo) * s;
}

// ---- Escuridao: quanto mais longe da camera, mais escura a cor (sem shaders)

// Fator de luz entre 0 (preto) e 1 (cor original) para um ponto do chao.
// Usa 1 - (distancia/raio)^2: perto fica bem iluminado e a luz cai rapido
// perto do limite do raio, como uma lanterna.
static float luzEm(float x, float z) {
    float dx = x - cam.atual.x;
    float dz = z - cam.atual.z;
    float luz = 1.0f - (dx * dx + dz * dz) / (RAIO_VISAO * RAIO_VISAO);
    return fmaxf(luz, 0.0f);
}

// Multiplica r, g e b pelo fator (0 a 1); a transparencia (a) nao muda
static Color escalarCor(Color cor, float fator) {
    fator = fminf(fmaxf(fator, 0.0f), 1.0f);
    return (Color){
        (unsigned char)(cor.r * fator),
        (unsigned char)(cor.g * fator),
        (unsigned char)(cor.b * fator),
        cor.a
    };
}

// Um vertice com a cor ja escurecida pela distancia dele ate a camera
static void vertice(Vector3 v, Color cor) {
    Color c = escalarCor(cor, luzEm(v.x, v.z));
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(v.x, v.y, v.z);
}

// Quadrilatero (dois triangulos) com a escuridao calculada em cada canto: a
// placa de video faz o degrade entre os cantos. Os cantos a, b, c, d vem em
// sentido anti-horario vistos do lado que deve aparecer (o outro e descartado).
static void desenharQuad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color cor) {
    rlBegin(RL_TRIANGLES);
        vertice(a, cor); vertice(b, cor); vertice(c, cor);
        vertice(a, cor); vertice(c, cor); vertice(d, cor);
    rlEnd();
}

// Retangulo deitado na altura y, de (x0, z0) ate (x1, z1).
// paraCima = true: visivel de cima (piso). false: visivel de baixo (teto).
static void desenharRetanguloDeitado(float x0, float z0, float x1, float z1, float y, bool paraCima, Color cor) {
    Vector3 a = { x0, y, z0 }, b = { x0, y, z1 }, c = { x1, y, z1 }, d = { x1, y, z0 };
    if (paraCima) desenharQuad(a, b, c, d, cor);
    else          desenharQuad(a, d, c, b, cor); // ordem inversa vira a face
}

// ---- Cenario: paredes, piso e teto -----------------------------------------

// Fora do mapa conta como parede (nao precisa desenhar a face para fora)
static bool ehParede3D(Mapa *mapa, int linha, int coluna) {
    if (linha < 0 || linha >= mapa->linhas || coluna < 0 || coluna >= mapa->colunas) return true;
    return mapa->matriz[linha][coluna] == '#';
}

// Forma do guia de marca na parede: circulo (1), meio circulo (2) ou anel (3),
// desenhada como um leque de triangulos no plano da face.
// (xEsq, zEsq) = canto esquerdo da face no chao; (dx, dz) = direcao ao longo
// dela; (nx, nz) = pequeno passo para fora da parede.
static void desenharFormaDaMarca(float xEsq, float zEsq, float dx, float dz, float nx, float nz, float sombra, int forma) {
    const float meio = 0.5f, altura = 0.33f, raio = 0.16f; // centro da forma na face e tamanho
    int fatias = (forma == 2) ? 12 : 24;                    // meio circulo = metade das 24 fatias
    int camadas = (forma == 3) ? 2 : 1;                     // anel = circulo + miolo da cor da parede

    for (int camada = 0; camada < camadas; camada++) {
        float r = (camada == 0) ? raio : raio * 0.55f;
        Color cor = escalarCor((camada == 0) ? COR_LARANJA_3D : COR_PAREDE_3D, sombra);
        float fora = 1.0f + (float)camada;                  // o miolo fica a frente do circulo
        float bx = xEsq + nx * fora, bz = zEsq + nz * fora; // canto da face, ja afastado da parede

        rlBegin(RL_TRIANGLES);
            for (int k = 0; k < fatias; k++) {
                float a0 = 2.0f * PI * (float)k / 24.0f, a1 = 2.0f * PI * (float)(k + 1) / 24.0f;
                float s0 = meio + r * cosf(a0), s1 = meio + r * cosf(a1);
                vertice((Vector3){ bx + dx * meio, altura, bz + dz * meio }, cor);
                vertice((Vector3){ bx + dx * s0, altura + r * sinf(a0), bz + dz * s0 }, cor);
                vertice((Vector3){ bx + dx * s1, altura + r * sinf(a1), bz + dz * s1 }, cor);
            }
        rlEnd();
    }
}

// Uma face vertical de parede, com efeitos. (xEsq, zEsq) e (xDir, zDir) sao
// os cantos no chao, da esquerda para a direita vistos de fora da parede.
// sombra < 1 escurece as faces leste/oeste (volume nas quinas).
// De baixo para cima a face e feita de faixas empilhadas:
//   rodape laranja | parede | faixa de luz laranja | parede | friso escuro
// A faixa de luz e a iluminacao de emergencia do predio: pulsa devagar e
// nunca apaga de todo, entao marca o caminho dos corredores no escuro.
// enfeite: 0 = nada; 1, 2 ou 3 = uma forma do guia de marca na parede.
static void desenharFaceParede(float xEsq, float zEsq, float xDir, float zDir, float sombra, int enfeite) {
    static const float ALTURAS[] = { 0.0f, 0.08f, 0.58f, 0.62f, 0.90f, ALTURA_PAREDE };
    const Color cores[] = { COR_RODAPE_3D, COR_PAREDE_3D, COR_LARANJA_3D, COR_PAREDE_3D, COR_FRISO_3D };

    for (int k = 0; k < 5; k++) {
        Vector3 a = { xEsq, ALTURAS[k], zEsq },     b = { xDir, ALTURAS[k], zDir };
        Vector3 c = { xDir, ALTURAS[k + 1], zDir }, d = { xEsq, ALTURAS[k + 1], zEsq };
        if (k != 2) {
            desenharQuad(a, b, c, d, escalarCor(cores[k], sombra));
            continue;
        }
        // Faixa de luz: brilho proprio (pulso), sem escurecer com a distancia
        float pulso = 0.45f + 0.15f * sinf((float)GetTime() * 2.0f);
        Color luz = escalarCor(COR_LARANJA_3D, fmaxf(pulso, luzEm((xEsq + xDir) / 2.0f, (zEsq + zDir) / 2.0f)));
        rlBegin(RL_TRIANGLES);
            rlColor4ub(luz.r, luz.g, luz.b, 255);
            rlVertex3f(a.x, a.y, a.z); rlVertex3f(b.x, b.y, b.z); rlVertex3f(c.x, c.y, c.z);
            rlVertex3f(a.x, a.y, a.z); rlVertex3f(c.x, c.y, c.z); rlVertex3f(d.x, d.y, d.z);
        rlEnd();
    }

    // Os detalhes abaixo ficam 4 mm a frente da parede (direcao "para fora"),
    // senao a placa de video nao sabe qual dos dois desenhar por cima
    float dx = xDir - xEsq, dz = zDir - zEsq;       // ao longo da face
    float nx = -dz * 0.004f, nz = dx * 0.004f;      // para fora da parede

    // Emenda entre paineis: risco escuro na borda esquerda da face
    Vector3 e0 = { xEsq + nx, 0.08f, zEsq + nz }, e1 = { xEsq + dx * 0.02f + nx, 0.08f, zEsq + dz * 0.02f + nz };
    Vector3 e2 = { e1.x, 0.90f, e1.z },           e3 = { e0.x, 0.90f, e0.z };
    desenharQuad(e0, e1, e2, e3, escalarCor(COR_FRISO_3D, sombra));

    if (enfeite != 0) desenharFormaDaMarca(xEsq, zEsq, dx, dz, nx, nz, sombra, enfeite);
}

// '#' vira um cubo, mas so desenhamos as faces que encostam em piso:
// as faces entre duas paredes nunca aparecem, entao nao gastamos tempo nelas.
static void desenharParede3D(Mapa *mapa, int i, int j) {
    float x0 = (float)j, x1 = x0 + 1.0f;
    float z0 = (float)i, z1 = z0 + 1.0f;
    // Uma a cada 7 paredes (em diagonal) ganha uma forma da marca: 1, 2 ou 3
    int enfeite = ((i * 5 + j * 3) % 7 == 0) ? 1 + (i + j) % 3 : 0;

    if (!ehParede3D(mapa, i + 1, j)) desenharFaceParede(x0, z1, x1, z1, 1.0f, enfeite);  // sul
    if (!ehParede3D(mapa, i - 1, j)) desenharFaceParede(x1, z0, x0, z0, 1.0f, enfeite);  // norte
    if (!ehParede3D(mapa, i, j + 1)) desenharFaceParede(x1, z1, x1, z0, 0.75f, enfeite); // leste
    if (!ehParede3D(mapa, i, j - 1)) desenharFaceParede(x0, z0, x0, z1, 0.75f, enfeite); // oeste
}

// Piso e teto de uma casa andavel.
// Corredor: cinza com uma placa mais clara no meio. Sala: xadrez marrom.
static void desenharPisoTeto3D(int i, int j, bool corredor) {
    float x0 = (float)j, x1 = x0 + 1.0f;
    float z0 = (float)i, z1 = z0 + 1.0f;
    Color sala = ((i + j) % 2 == 0) ? COR_SALA_A_3D : COR_SALA_B_3D; // xadrez

    desenharRetanguloDeitado(x0, z0, x1, z1, 0.0f, true, corredor ? COR_CORREDOR_3D : sala);

    if (corredor) {
        // Placa um pouquinho acima do chao (0.005) para nao "brigar" com ele na tela
        float m = 0.08f;
        desenharRetanguloDeitado(x0 + m, z0 + m, x1 - m, z1 - m, 0.005f, true, COR_PLACA_3D);
    }

    desenharRetanguloDeitado(x0, z0, x1, z1, ALTURA_PAREDE, false, COR_TETO_3D);
}

// Elipse deitada no chao, na altura y, com centro (cx, cz) e raios rx/rz.
// E um "leque" de triangulos saindo do centro; a ordem dos cantos e
// anti-horaria vista de cima, para a face aparecer por cima.
static void desenharElipseDeitada(float cx, float cz, float rx, float rz, float y, Color cor) {
    const int fatias = 24;
    rlBegin(RL_TRIANGLES);
        for (int k = 0; k < fatias; k++) {
            float a0 = 2.0f * PI * (float)k / (float)fatias;
            float a1 = 2.0f * PI * (float)(k + 1) / (float)fatias;
            vertice((Vector3){ cx, y, cz }, cor);
            vertice((Vector3){ cx + rx * cosf(a1), y, cz + rz * sinf(a1) }, cor);
            vertice((Vector3){ cx + rx * cosf(a0), y, cz + rz * sinf(a0) }, cor);
        }
    rlEnd();
}

// Logo do CESAR em claro sobre o tapete laranja, centralizada no retangulo de
// casas que comeca em (i, j). E a mesma do mapa 2D, feita de elipses; cada
// camada fica um pouquinho acima da anterior para a placa de video saber
// qual desenhar por cima. O "cima" da logo aponta para o norte do mapa.
static void desenharLogoTapete3D(int i, int j, int largura, int altura) {
    float cx = (float)j + (float)largura / 2.0f, cz = (float)i + (float)altura / 2.0f;
    float rx = fminf((float)largura * 0.35f, (float)altura * 0.45f), rz = rx / 2.0f;
    float az = cz - rz * 0.28f; // os aneis ficam um pouco acima (norte) do centro

    desenharElipseDeitada(cx, cz, rx, rz, 0.010f, COR_CLARO_3D);
    desenharElipseDeitada(cx, az, rx * 0.62f, rz * 0.58f, 0.012f, COR_LARANJA_3D);
    desenharElipseDeitada(cx, az, rx * 0.44f, rz * 0.38f, 0.014f, COR_CLARO_3D);
    desenharElipseDeitada(cx, az, rx * 0.26f, rz * 0.20f, 0.016f, COR_LARANJA_3D);
    // Abertura dos aneis em cima
    desenharRetanguloDeitado(cx - rx * 0.08f, az - rz * 0.62f, cx + rx * 0.08f, az - rz * 0.17f, 0.018f, true, COR_CLARO_3D);
}

// ---- Objetos: moveis, escada, professor ------------------------------------
// Cada objeto e pequeno, entao usa uma luz so (a do centro da casa).

// Cubo com a cor escurecida e as arestas marcadas com uma cor mais escura
static void cuboComBorda(Vector3 meio, Vector3 tamanho, Color cor, Color borda, float luz) {
    DrawCubeV(meio, tamanho, escalarCor(cor, luz));
    DrawCubeWires(meio, tamanho.x, tamanho.y, tamanho.z, escalarCor(borda, luz));
}

// Mesa 'M': tampo de madeira e quatro pernas
static void desenharMesa3D(float cx, float cz, float luz) {
    cuboComBorda((Vector3){ cx, 0.43f, cz }, (Vector3){ 0.84f, 0.06f, 0.64f }, COR_MADEIRA_3D, COR_MADEIRA_E_3D, luz);

    // Pernas nos 4 cantos: k % 2 escolhe esquerda/direita e k < 2, frente/tras
    for (int k = 0; k < 4; k++) {
        Vector3 perna = { cx + (k % 2 ? 0.36f : -0.36f), 0.2f, cz + (k < 2 ? -0.26f : 0.26f) };
        DrawCube(perna, 0.06f, 0.4f, 0.06f, escalarCor(COR_MADEIRA_E_3D, luz));
    }
}

// Computador 'C': bloco baixo de metal com um monitor em cima
static void desenharComputador3D(float cx, float cz, float luz) {
    Color metalEscuro = escalarCor(COR_METAL_E_3D, luz);

    cuboComBorda((Vector3){ cx, 0.22f, cz }, (Vector3){ 0.84f, 0.44f, 0.64f }, COR_METAL_3D, COR_METAL_E_3D, luz);
    DrawCube((Vector3){ cx, 0.47f, cz }, 0.06f, 0.06f, 0.06f, metalEscuro); // pe do monitor
    DrawCube((Vector3){ cx, 0.62f, cz }, 0.5f, 0.32f, 0.08f, metalEscuro);  // carcaca

    // A tela e um pouco mais grossa que a carcaca, entao aparece dos dois lados.
    // Ela emite luz: nunca fica mais escura que 50%.
    DrawCube((Vector3){ cx, 0.63f, cz }, 0.42f, 0.24f, 0.1f, escalarCor(COR_TELA_3D, fmaxf(luz, 0.5f)));
}

// Escada: 4 degraus subindo em direcao ao norte (-z). Cinza bloqueada, verde liberada.
static void desenharEscada3D(Mapa *mapa, bool liberada) {
    if (mapa->saida.linha < 0) return;

    float cx = centro(mapa->saida.coluna);
    float cz = centro(mapa->saida.linha);
    float luz = luzEm(cx, cz);
    if (luz <= 0.0f) return;              // longe demais: nao desenha
    if (liberada) luz = fmaxf(luz, 0.6f); // verde liberada brilha no escuro

    Color claro  = liberada ? (Color){ 0, 210, 90, 255 } : (Color){ 135, 135, 140, 255 };
    Color escuro = liberada ? (Color){ 0, 105, 45, 255 } : (Color){  65,  65,  70, 255 };

    for (int k = 0; k < 4; k++) {
        float altura = 0.1f * (float)(k + 1);          // cada degrau e 0.1 mais alto
        float z = cz + 0.375f - (float)k * 0.25f;      // e fica 0.25 mais para tras
        Color cor = (k % 2 == 0) ? claro : escuro;     // degraus alternam de cor
        cuboComBorda((Vector3){ cx, altura / 2.0f, z }, (Vector3){ 0.9f, altura, 0.25f }, cor, escuro, luz);
    }
}

// Professor: seleciona dinamicamente a pose (0 = Normal, 1 = Joinha) no array bidimensional [3][2]
static void desenharProfessor3D(Professor *professor, Texture2D texturas[3][2], Camera3D camera) {
    if (professor == NULL || professor->posicao.linha < 0) return;

    float cx = centro(professor->posicao.coluna);
    float cz = centro(professor->posicao.linha);
    float luz = luzEm(cx, cz);
    if (luz <= 0.0f) return;

    int profID = professor->id;                 // ID do professor (0, 1 ou 2)
    int poseIndex = professor->ajudado ? 1 : 0; // 0 = Normal, 1 = Joinha

    Texture2D tex = texturas[profID][poseIndex];

    if (tex.id != 0) {
        Vector3 posSprite = { cx, 0.5f, cz };
        DrawBillboard(camera, tex, posSprite, 1.0f, escalarCor(WHITE, luz));
    } else {
        Color cor = escalarCor(professor->ajudado ? GRAY : PURPLE, luz);
        DrawCylinder((Vector3){ cx, 0.0f, cz }, 0.18f, 0.24f, 0.6f, 16, cor);
        DrawSphere((Vector3){ cx, 0.72f, cz }, 0.12f, cor);
    }

    if (!professor->ajudado) {
        Color amarelo = escalarCor(YELLOW, fmaxf(luz, 0.7f));
        DrawCube((Vector3){ cx, 0.95f, cz }, 0.04f, 0.08f, 0.04f, amarelo); // traco
        DrawCube((Vector3){ cx, 0.87f, cz }, 0.04f, 0.03f, 0.04f, amarelo); // ponto
    }
}

void desenharNickProfessor3D(Professor *professor) {
    if (professor == NULL || professor->posicao.linha < 0) return;

    float cx = (float)professor->posicao.coluna + 0.5f;
    float cz = (float)professor->posicao.linha + 0.5f;
    if (luzEm(cx, cz) <= 0.05f) return;

    Camera3D camera = { 0 };
    camera.position = (Vector3){ cam.atual.x, ALTURA_OLHOS, cam.atual.z };
    camera.target   = (Vector3){ cam.atual.x + cosf(cam.atual.angulo), ALTURA_OLHOS, cam.atual.z + sinf(cam.atual.angulo) };
    camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy     = CAMPO_VISAO;
    camera.projection = CAMERA_PERSPECTIVE;

    Vector3 posCabeca = { cx, 1.15f, cz };

    float dirX = cosf(cam.atual.angulo);
    float dirZ = sinf(cam.atual.angulo);
    float profDX = cx - cam.atual.x;
    float profDZ = cz - cam.atual.z;

    if ((profDX * dirX + profDZ * dirZ) <= 0.2f) return;

    Vector2 posTela = GetWorldToScreen(posCabeca, camera);

    if (posTela.x >= 0 && posTela.x <= GetScreenWidth() &&
        posTela.y >= 0 && posTela.y <= GetScreenHeight()) {

        float e = hudEscala();
        int tamanhoFonte = (int)(14.0f * e);
        if (tamanhoFonte < 10) tamanhoFonte = 10;

        int larguraTexto = MeasureText(professor->nome, tamanhoFonte);
        int posX = (int)posTela.x - (larguraTexto / 2);
        int posY = (int)posTela.y;

        DrawRectangle(posX - 6, posY - 2, larguraTexto + 12, tamanhoFonte + 4, Fade(BLACK, 0.65f));

        Color corNick = professor->ajudado ? LIGHTGRAY : YELLOW;
        DrawText(professor->nome, posX, posY, tamanhoFonte, corNick);
    }
}

// ---- Cena completa ---------------------------------------------------------

// Desenha uma casa do mapa: parede, ou piso/teto com o movel que estiver nela
static void desenharCasa3D(Mapa *mapa, int i, int j) {
    char c = mapa->matriz[i][j];

    if (c == '#') {
        desenharParede3D(mapa, i, j);
        return;
    }

    desenharPisoTeto3D(i, j, c == '.'); // moveis ficam em salas: piso de sala
    if (mapaTemTapete(mapa, i, j)) {
        // Tapete: placa laranja um pouco acima do piso e, no canto, a logo inteira
        desenharRetanguloDeitado((float)j, (float)i, (float)j + 1.0f, (float)i + 1.0f, 0.008f, true, COR_LARANJA_3D);
        int largura, altura;
        if (mapaCantoTapete(mapa, i, j, &largura, &altura)) desenharLogoTapete3D(i, j, largura, altura);
    }

    float cx = centro(j), cz = centro(i);
    if (c == 'M') desenharMesa3D(cx, cz, luzEm(cx, cz));
    if (c == 'C') desenharComputador3D(cx, cz, luzEm(cx, cz));
}

static void desenharRobos3D(Jogo *jogo, Camera3D camera) {
    if (jogo == NULL) return;

    Robo *r = jogo->robos;
    while (r != NULL) {
        if (r->estado == ESTADO_DESLIGADO) {
            r = r->next;
            continue;
        }

        float cx = r->posicaoReal.x + 0.5f;
        float cz = r->posicaoReal.y + 0.5f;

        float luz = luzEm(cx, cz);
        if (luz <= 0.0f) {
            r = r->next;
            continue;
        }

        // Escolhe o sprite conforme a direcao do robo em relacao a camera:
        // [0] = Costas, [1] = Frente, [2] = Esquerda, [3] = Direita
        float toCamX = cam.atual.x - cx;
        float toCamZ = cam.atual.z - cz;
        float distCam = sqrtf(toCamX * toCamX + toCamZ * toCamZ);

        int idxTex = 1; // Padrao: Frente
        float flipX = 1.0f;

        if (distCam > 0.001f && (r->direcao.x != 0.0f || r->direcao.y != 0.0f)) {
            toCamX = toCamX / distCam;
            toCamZ = toCamZ / distCam;

            float dot = r->direcao.x * toCamX + r->direcao.y * toCamZ;
            if (dot > 0.25f) {
                idxTex = 1; // Frente (vindo na direcao do jogador)
            } else if (dot < -0.35f) {
                idxTex = 0; // Costas (indo para longe do jogador)
            } else {
                float cross = r->direcao.x * toCamZ - r->direcao.y * toCamX;
                if (cross > 0.0f) {
                    idxTex = 3; // Direita na tela
                } else {
                    idxTex = 2; // Esquerda na tela
                }

                // O robo vermelho e o roxo usam a mesma textura de lado nos indices 2 e 3,
                // entao espelha quando for para o outro lado
                if ((r->tipo == ROBO_VERMELHO || r->tipo == ROBO_ROXO) && cross < 0.0f) {
                    flipX = -1.0f;
                }
            }
        }

        Texture2D tex;
        Color corFallback;
        if (r->tipo == ROBO_ROXO) {
            tex = jogo->texRoboRoxo[idxTex];
            corFallback = PURPLE;
        } else if (r->tipo == ROBO_VERMELHO) {
            tex = jogo->texRoboVermelho[idxTex];
            corFallback = ORANGE;
        } else if (r->tipo == ROBO_CINZA) {
            if (idxTex == 1 && ((int)(GetTime() * 4.0) % 2 == 1)) {
                tex = jogo->texRoboCinza[4];
            } else {
                tex = jogo->texRoboCinza[idxTex];
            }
            corFallback = GRAY;
        } else {
            tex = jogo->texRoboLaranja[idxTex];
            corFallback = RED;
        }

        Color baseTint = WHITE;
        if (r->estado == ESTADO_FUGINDO) {
            baseTint = SKYBLUE;
        }
        Color tint = escalarCor(baseTint, luz);

        if (tex.id != 0) {
            Vector3 posSprite = { cx, 0.45f, cz };
            Rectangle srcRec = { 0.0f, 0.0f, (float)tex.width * flipX, (float)tex.height };
            Vector2 tamanho = { 0.85f, 0.85f };
            DrawBillboardRec(camera, tex, srcRec, posSprite, tamanho, tint);
        } else {
            Color c = escalarCor(corFallback, luz);
            if (r->estado == ESTADO_FUGINDO) {
                c = escalarCor(SKYBLUE, luz);
            }
            DrawCylinder((Vector3){ cx, 0.0f, cz }, 0.22f, 0.22f, 0.7f, 16, c);
        }

        r = r->next;
    }
}
// Monta a camera nos olhos do jogador e desenha, nesta ordem: cenario
// (paredes, piso, teto, moveis), escada, professor e baterias
void desenharCena3D(Jogo *jogo) {
    if (jogo == NULL || jogo->mapa.matriz == NULL) return;

    Mapa *mapa = &jogo->mapa;
    Pose olho = cam.atual;

    // Camera nos olhos do jogador, olhando 1 casa para a frente na direcao do angulo
    Camera3D camera = {
        .position   = { olho.x, ALTURA_OLHOS, olho.z },
        .target     = { olho.x + cosf(olho.angulo), ALTURA_OLHOS, olho.z + sinf(olho.angulo) },
        .up         = { 0.0f, 1.0f, 0.0f },
        .fovy       = CAMPO_VISAO,
        .projection = CAMERA_PERSPECTIVE
    };

    BeginMode3D(camera);

        // So desenha as casas dentro do circulo de visao. O raio tem 1 casa de
        // folga para que paredes que cruzam a borda aparecam ate escurecer.
        int raio = (int)RAIO_VISAO + 1;
        float raioQuadrado = (RAIO_VISAO + 1.0f) * (RAIO_VISAO + 1.0f);
        int linhaCam = (int)olho.z, colunaCam = (int)olho.x;

        for (int i = linhaCam - raio; i <= linhaCam + raio; i++) {
            for (int j = colunaCam - raio; j <= colunaCam + raio; j++) {
                if (i < 0 || i >= mapa->linhas || j < 0 || j >= mapa->colunas) continue;

                float dx = centro(j) - olho.x, dz = centro(i) - olho.z;
                if (dx * dx + dz * dz > raioQuadrado) continue;

                desenharCasa3D(mapa, i, j);
            }
        }

        desenharEscada3D(mapa, jogo->professor.ajudado);
        desenharProfessor3D(&jogo->professor, jogo->texturasProfessores, camera);
        
        // Desenha as baterias em 3D centralizadas via itens.c utilizando a imagem 2D
        desenhar_item_3d(jogo, camera);
        desenharRobos3D(jogo, camera);

    EndMode3D();
}

// Mapa 2D pequeno no canto superior direito, para o jogador nao se perder no 3D
void desenharMinimapa(Jogo *jogo) {
    if (jogo == NULL || jogo->mapa.matriz == NULL) return;

    // Casa e margem crescem junto com a janela (minimo de 3px por casa)
    float escala = hudEscala();
    int t = (int)(CASA_MINIMAPA * escala);
    if (t < 3) t = 3;
    int margem = (int)(MARGEM_MINIMAPA * escala);
    int largura = jogo->mapa.colunas * t;
    int altura  = jogo->mapa.linhas * t;

    // Canto superior direito da tela
    Vector2 origem = { (float)(GetScreenWidth() - largura - margem), (float)margem };

    // Moldura escura atras do minimapa
    DrawRectangle((int)origem.x - 4, (int)origem.y - 4, largura + 8, altura + 8, Fade(BLACK, 0.75f));
    DrawRectangleLines((int)origem.x - 4, (int)origem.y - 4, largura + 8, altura + 8, Fade(RAYWHITE, 0.5f));

    // As mesmas funcoes do modo 2D, so que com outra origem e casas pequenas
    desenharMapa2D(&jogo->mapa, origem, t);
    desenharSaida2D(&jogo->mapa, origem, t, jogo->professor.ajudado);
    desenhar_item(jogo, origem, t);
    desenhar_professor(jogo, origem, t);
    desenharRobos(jogo, origem, t);
    desenharSetaJogador2D(&jogo->jogador, origem, t);
}
