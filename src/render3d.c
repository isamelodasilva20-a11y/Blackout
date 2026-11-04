#include <math.h>
#include <stddef.h>
#include "raylib.h"
#include "rlgl.h"   // camada de baixo nivel que vem junto com a raylib
#include "render3d.h"
#include "mapa.h"
#include "itens.h"
#include "professor.h"
#include "jogador.h"
#include "hud.h"

// Medidas do mundo 3D (1.0 = tamanho de uma casa do mapa)
#define ALTURA_PAREDE   1.0f
#define ALTURA_RODAPE   0.08f
#define ALTURA_OLHOS    0.5f
#define CAMPO_VISAO     70.0f  // abertura da camera, em graus
#define RAIO_VISAO      7.0f   // ate quantas casas de distancia ainda se enxerga algo
#define TEMPO_TRANSICAO 0.15f  // duracao da animacao de andar/girar, em segundos

// Minimapa (medidas no tamanho base da janela; sao multiplicadas pela escala)
#define CASA_MINIMAPA   6
#define MARGEM_MINIMAPA 10

// Cores do mundo 3D (antes de escurecer pela distancia)
#define COR_PAREDE_3D    (Color){  60,  92, 150, 255 }
#define COR_RODAPE_3D    (Color){  25,  35,  60, 255 }
#define COR_CORREDOR_3D  (Color){  62,  64,  74, 255 }
#define COR_PLACA_3D     (Color){  74,  76,  88, 255 } // placa mais clara no meio do corredor
#define COR_SALA_A_3D    (Color){ 100,  80,  60, 255 }
#define COR_SALA_B_3D    (Color){  86,  69,  51, 255 }
#define COR_TETO_3D      (Color){  22,  22,  28, 255 }
#define COR_MADEIRA_3D   (Color){ 130,  88,  52, 255 }
#define COR_MADEIRA_E_3D (Color){  80,  52,  30, 255 }
#define COR_METAL_3D     (Color){ 100, 103, 115, 255 }
#define COR_METAL_E_3D   (Color){  45,  47,  55, 255 }
#define COR_TELA_3D      (Color){  80, 170, 235, 255 }
#define COR_BATERIA_3D   (Color){  40, 120, 255, 255 }

// ---- Camera ----------------------------------------------------------------

// Uma "pose" da camera: onde ela esta no chao (x, z) e para onde olha (angulo)
typedef struct {
    float x, z, angulo;
} Pose;

 * Estado da camera. A logica do jogo pula de casa em casa, mas a camera
 * "desliza": ela guarda de onde saiu (de...) e para onde vai (para...) e,
 * a cada quadro, anda um pouco entre os dois (progresso de 0 ate 1).
 */
typedef struct {
    bool iniciada;
    float x, z, angulo;             // onde a camera esta agora
    float deX, deZ, deAngulo;       // onde a animacao comecou
    float paraX, paraZ, paraAngulo; // onde a animacao termina
    float progresso;                // 0 = comecando, 1 = terminou
    Posicao posicaoAlvo;            // ultima casa do jogador que a camera viu
    Direcao direcaoAlvo;            // ultima direcao do jogador que a camera viu
} CameraJogo;

// Comeca zerada, na casa (0,0), que e sempre parede: na primeira atualizacao
// o jogador esta longe, e a regra do "pulo grande" ja coloca a camera nele.
static CameraJogo cam = { 0 };

static float anguloDaDirecao(Direcao direcao) {
    switch (direcao) {
        case DIR_DIREITA:  return 0.0f;
        case DIR_BAIXO:    return PI / 2.0f;
        case DIR_ESQUERDA: return PI;
        case DIR_CIMA:     return -PI / 2.0f;
    }
    return 0.0f;
}

void render3dReiniciarCamera(Jogo *jogo) {
    if (jogo == NULL) return;

    cam.x = cam.deX = cam.paraX = (float)jogo->jogador.posicao.coluna + 0.5f;
    cam.z = cam.deZ = cam.paraZ = (float)jogo->jogador.posicao.linha + 0.5f;
    cam.angulo = cam.deAngulo = cam.paraAngulo = anguloDaDirecao(jogo->jogador.direcao);
    cam.progresso = 1.0f;
    cam.posicaoAlvo = jogo->jogador.posicao;
    cam.direcaoAlvo = jogo->jogador.direcao;
}

void render3dAtualizarCamera(Jogo *jogo, float dt) {
    if (jogo == NULL) return;

    Posicao p = jogo->jogador.posicao;
    Direcao d = jogo->jogador.direcao;

    if (p.linha != cam.posicaoAlvo.linha || p.coluna != cam.posicaoAlvo.coluna || d != cam.direcaoAlvo) {
        Pose alvo = poseDoJogador(&jogo->jogador);

        if (fabsf(novoX - cam.x) + fabsf(novoZ - cam.z) > 1.5f) {
            render3dReiniciarCamera(jogo);
            return;
        }

        cam.deX = cam.x;
        cam.deZ = cam.z;
        cam.deAngulo = cam.angulo;
        cam.paraX = novoX;
        cam.paraZ = novoZ;

        float diferenca = anguloDaDirecao(d) - cam.angulo;
        while (diferenca >  PI) diferenca -= 2.0f * PI;
        while (diferenca < -PI) diferenca += 2.0f * PI;
        cam.paraAngulo = cam.angulo + diferenca;

        cam.progresso = 0.0f;
        cam.posicaoAlvo = p;
        cam.direcaoAlvo = d;
    }

    if (cam.progresso >= 1.0f) return; // parada: nada para animar

    cam.progresso = fminf(cam.progresso + dt / TEMPO_TRANSICAO, 1.0f);

        float s = cam.progresso * cam.progresso * (3.0f - 2.0f * cam.progresso);
        cam.x = cam.deX + (cam.paraX - cam.deX) * s;
        cam.z = cam.deZ + (cam.paraZ - cam.deZ) * s;
        cam.angulo = cam.deAngulo + (cam.paraAngulo - cam.deAngulo) * s;
    }
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

static Color escalarCor(Color cor, float fator) {
    fator = fminf(fmaxf(fator, 0.0f), 1.0f);
    return (Color){
        (unsigned char)(cor.r * fator),
        (unsigned char)(cor.g * fator),
        (unsigned char)(cor.b * fator),
        cor.a
    };
}

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

// Retangulo deitado na altura y, de (x0, z0) ate (x1, z1).
// paraCima = true: visivel de cima (piso). false: visivel de baixo (teto).
static void desenharRetanguloDeitado(float x0, float z0, float x1, float z1, float y, bool paraCima, Color cor) {
    Vector3 a = { x0, y, z0 }, b = { x0, y, z1 }, c = { x1, y, z1 }, d = { x1, y, z0 };
    if (paraCima) desenharQuad(a, b, c, d, cor);
    else          desenharQuad(a, d, c, b, cor); // ordem inversa vira a face
}

// ---- Cenario: paredes, piso e teto -----------------------------------------

static bool ehParede3D(Mapa *mapa, int linha, int coluna) {
    if (linha < 0 || linha >= mapa->linhas || coluna < 0 || coluna >= mapa->colunas) return true;
    return mapa->matriz[linha][coluna] == '#';
}

// Uma face vertical de parede, com rodape escuro embaixo. (xEsq, zEsq) e
// (xDir, zDir) sao os cantos no chao, da esquerda para a direita vistos de
// fora da parede. sombra < 1 escurece as faces leste/oeste (volume nas quinas).
static void desenharFaceParede(float xEsq, float zEsq, float xDir, float zDir, float sombra) {
    Vector3 esqChao   = { xEsq, 0.0f, zEsq },          dirChao   = { xDir, 0.0f, zDir };
    Vector3 esqRodape = { xEsq, ALTURA_RODAPE, zEsq }, dirRodape = { xDir, ALTURA_RODAPE, zDir };
    Vector3 esqTopo   = { xEsq, ALTURA_PAREDE, zEsq }, dirTopo   = { xDir, ALTURA_PAREDE, zDir };

    desenharQuad(esq, dir, dirRodape, esqRodape, rodape);
    desenharQuad(esqRodape, dirRodape, dirTopo, esqTopo, parede);
}

static void desenharParede3D(Mapa *mapa, int i, int j) {
    float x0 = (float)j, x1 = x0 + 1.0f;
    float z0 = (float)i, z1 = z0 + 1.0f;

    if (!ehParede3D(mapa, i + 1, j)) desenharFaceParede((Vector3){ x0, 0, z1 }, (Vector3){ x1, 0, z1 }, 1.0f);
    if (!ehParede3D(mapa, i - 1, j)) desenharFaceParede((Vector3){ x1, 0, z0 }, (Vector3){ x0, 0, z0 }, 1.0f);
    if (!ehParede3D(mapa, i, j + 1)) desenharFaceParede((Vector3){ x1, 0, z1 }, (Vector3){ x1, 0, z0 }, 0.75f);
    if (!ehParede3D(mapa, i, j - 1)) desenharFaceParede((Vector3){ x0, 0, z0 }, (Vector3){ x0, 0, z1 }, 0.75f);
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

// ---- Objetos: moveis, escada, professor e baterias -------------------------
// Cada objeto e pequeno, entao usa uma luz so (a do centro da casa).

// Cubo com a cor escurecida e as arestas marcadas com uma cor mais escura
static void cuboComBorda(Vector3 meio, Vector3 tamanho, Color cor, Color borda, float luz) {
    DrawCubeV(meio, tamanho, escalarCor(cor, luz));
    DrawCubeWires(meio, tamanho.x, tamanho.y, tamanho.z, escalarCor(borda, luz));
}

static void desenharMesa3D(int i, int j) {
    float cx = (float)j + 0.5f, cz = (float)i + 0.5f;
    float luz = luzEm(cx, cz);

    cuboComBorda((Vector3){ cx, 0.43f, cz }, (Vector3){ 0.84f, 0.06f, 0.64f }, COR_MADEIRA_3D, COR_MADEIRA_E_3D, luz);

    for (int k = 0; k < 4; k++) {
        Vector3 perna = { cx + (k % 2 ? 0.36f : -0.36f), 0.2f, cz + (k < 2 ? -0.26f : 0.26f) };
        DrawCube(perna, 0.06f, 0.4f, 0.06f, escalarCor(COR_MADEIRA_E_3D, luz));
    }
}

static void desenharComputador3D(int i, int j) {
    float cx = (float)j + 0.5f, cz = (float)i + 0.5f;
    float luz = luzEm(cx, cz);

    cuboComBorda((Vector3){ cx, 0.22f, cz }, (Vector3){ 0.84f, 0.44f, 0.64f }, COR_METAL_3D, COR_METAL_E_3D, luz);
    DrawCube((Vector3){ cx, 0.47f, cz }, 0.06f, 0.06f, 0.06f, escalarCor(COR_METAL_E_3D, luz));
    DrawCube((Vector3){ cx, 0.62f, cz }, 0.5f, 0.32f, 0.08f, escalarCor(COR_METAL_E_3D, luz));
    DrawCube((Vector3){ cx, 0.63f, cz }, 0.42f, 0.24f, 0.1f, escalarCor(COR_TELA_3D, fmaxf(luz, 0.5f)));
}

static void desenharEscada3D(Mapa *mapa, bool liberada) {
    if (mapa->saida.linha < 0) return;

    float cx = centro(mapa->saida.coluna);
    float cz = centro(mapa->saida.linha);
    float luz = luzEm(cx, cz);
    if (luz <= 0.0f) return;
    if (liberada) luz = fmaxf(luz, 0.6f);

    Color claro  = liberada ? (Color){ 0, 210, 90, 255 } : (Color){ 135, 135, 140, 255 };
    Color escuro = liberada ? (Color){ 0, 105, 45, 255 } : (Color){  65,  65,  70, 255 };

    for (int k = 0; k < 4; k++) {
        float altura = 0.1f * (float)(k + 1);
        float z = cz + 0.375f - (float)k * 0.25f;
        cuboComBorda((Vector3){ cx, altura / 2.0f, z }, (Vector3){ 0.9f, altura, 0.25f },
                     k % 2 == 0 ? claro : escuro, escuro, luz);
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
    camera.position = (Vector3){ cam.x, ALTURA_OLHOS, cam.z };
    camera.target   = (Vector3){ cam.x + cosf(cam.angulo), ALTURA_OLHOS, cam.z + sinf(cam.angulo) };
    camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy     = CAMPO_VISAO;
    camera.projection = CAMERA_PERSPECTIVE;

    Vector3 posCabeca = { cx, 1.15f, cz };

    float dirX = cosf(cam.angulo);
    float dirZ = sinf(cam.angulo);
    float profDX = cx - cam.x;
    float profDZ = cz - cam.z;

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

static void desenharBaterias3D(Item *itens) {
    float tempo = (float)GetTime();
    // sin() faz o valor ir e voltar: o brilho de todas as baterias pulsa junto
    float pulso = 0.5f + 0.5f * sinf(tempo * 4.0f);

    for (Item *atual = itens; atual != NULL; atual = atual->next) {
        float cx = centro(atual->posicao.coluna);
        float cz = centro(atual->posicao.linha);
        float luz = luzEm(cx, cz);
        if (luz <= 0.0f) continue;

        float brilho = fmaxf(luz, 0.55f);
        // Sobe e desce; somar a coluna faz cada bateria balancar num ritmo diferente
        float y = 0.22f + 0.04f * sinf(tempo * 3.0f + (float)atual->posicao.coluna);

        DrawCylinder((Vector3){ cx, y - 0.11f, cz }, 0.07f, 0.07f, 0.22f, 12, escalarCor(COR_BATERIA_3D, brilho));
        DrawCylinder((Vector3){ cx, y + 0.11f, cz }, 0.03f, 0.03f, 0.04f, 8, escalarCor(LIGHTGRAY, brilho));
        DrawSphere((Vector3){ cx, y, cz }, 0.2f + 0.04f * pulso, Fade(SKYBLUE, (0.12f + 0.12f * pulso) * brilho));
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

    float cx = centro(j), cz = centro(i);
    if (c == 'M') desenharMesa3D(cx, cz, luzEm(cx, cz));
    if (c == 'C') desenharComputador3D(cx, cz, luzEm(cx, cz));
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

    float cx = centro(j), cz = centro(i);
    if (c == 'M') desenharMesa3D(cx, cz, luzEm(cx, cz));
    if (c == 'C') desenharComputador3D(cx, cz, luzEm(cx, cz));
}

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
                if (j < 0 || j >= mapa->colunas) continue;

                float dx = (float)j + 0.5f - cam.x, dz = (float)i + 0.5f - cam.z;
                if (dx * dx + dz * dz > (RAIO_VISAO + 1.0f) * (RAIO_VISAO + 1.0f)) continue;

                char c = mapa->matriz[i][j];
                if (c == '#') {
                    desenharParede3D(mapa, i, j);
                } else {
                    desenharPisoTeto3D(i, j, (c == 'M' || c == 'C') ? ' ' : c);
                    if (c == 'M') desenharMesa3D(i, j);
                    if (c == 'C') desenharComputador3D(i, j);
                }
            }
        }

        desenharEscada3D(mapa, jogo->professor.ajudado);
        desenharProfessor3D(&jogo->professor, jogo->texturasProfessores, camera);
        desenharBaterias3D(jogo->itens);

    EndMode3D();
}

void desenharMinimapa(Jogo *jogo) {
    if (jogo == NULL || jogo->mapa.matriz == NULL) return;

    float escala = hudEscala();
    int t = (int)(CASA_MINIMAPA * escala);
    if (t < 3) t = 3;
    int margem = (int)(MARGEM_MINIMAPA * escala);
    int largura = jogo->mapa.colunas * t;
    int altura  = jogo->mapa.linhas * t;

    Vector2 origem = { (float)(GetScreenWidth() - largura - margem), (float)margem };

    DrawRectangle((int)origem.x - 4, (int)origem.y - 4, largura + 8, altura + 8, Fade(BLACK, 0.75f));
    DrawRectangleLines((int)origem.x - 4, (int)origem.y - 4, largura + 8, altura + 8, Fade(RAYWHITE, 0.5f));

    desenharMapa2D(&jogo->mapa, origem, t);
    desenharSaida2D(&jogo->mapa, origem, t, jogo->professor.ajudado);
    desenhar_item(jogo->itens, origem, t);
    desenhar_professor(jogo, origem, t);
    desenharSetaJogador2D(&jogo->jogador, origem, t);
}
