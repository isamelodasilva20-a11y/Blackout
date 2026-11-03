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

/*
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

static CameraJogo cam = { 0 };

// Angulo (em radianos) para onde cada direcao aponta no plano x/z.
// 0 = olhando para +x (direita). Como z = linha, "baixo" no mapa e +z.
static float anguloDaDirecao(Direcao direcao) {
    switch (direcao) {
        case DIR_DIREITA:  return 0.0f;
        case DIR_BAIXO:    return PI / 2.0f;
        case DIR_ESQUERDA: return PI;
        case DIR_CIMA:     return -PI / 2.0f;
    }
    return 0.0f;
}

// Pula a camera direto para a casa e a direcao do jogador (sem animacao):
// usada ao ligar o 3D e quando o jogador "teleporta" (troca de andar)
void render3dReiniciarCamera(Jogo *jogo) {
    if (jogo == NULL) return;

    // +0.5 coloca a camera no meio da casa
    cam.x = cam.deX = cam.paraX = (float)jogo->jogador.posicao.coluna + 0.5f;
    cam.z = cam.deZ = cam.paraZ = (float)jogo->jogador.posicao.linha + 0.5f;
    cam.angulo = cam.deAngulo = cam.paraAngulo = anguloDaDirecao(jogo->jogador.direcao);
    cam.progresso = 1.0f;
    cam.posicaoAlvo = jogo->jogador.posicao;
    cam.direcaoAlvo = jogo->jogador.direcao;
    cam.iniciada = true;
}

// Chamada todo quadro: percebe quando o jogador mudou de casa ou de direcao
// e faz a camera deslizar ate la em TEMPO_TRANSICAO segundos
void render3dAtualizarCamera(Jogo *jogo, float dt) {
    if (jogo == NULL) return;
    if (!cam.iniciada) {
        render3dReiniciarCamera(jogo);
        return;
    }

    Posicao p = jogo->jogador.posicao;
    Direcao d = jogo->jogador.direcao;

    // O jogador mudou de casa ou de direcao: comeca uma nova animacao
    // a partir de onde a camera esta agora
    if (p.linha != cam.posicaoAlvo.linha || p.coluna != cam.posicaoAlvo.coluna || d != cam.direcaoAlvo) {
        float novoX = (float)p.coluna + 0.5f;
        float novoZ = (float)p.linha + 0.5f;

        // Pulo de mais de uma casa (ex.: troca de andar): vai direto, sem animar
        if (fabsf(novoX - cam.x) + fabsf(novoZ - cam.z) > 1.5f) {
            render3dReiniciarCamera(jogo);
            return;
        }

        cam.deX = cam.x;
        cam.deZ = cam.z;
        cam.deAngulo = cam.angulo;
        cam.paraX = novoX;
        cam.paraZ = novoZ;

        // Gira pelo caminho mais curto: a diferenca fica entre -180 e +180 graus.
        // Sem isso, ir de -90 (cima) para 180 (esquerda) daria 3/4 de volta.
        float diferenca = anguloDaDirecao(d) - cam.angulo;
        while (diferenca >  PI) diferenca -= 2.0f * PI;
        while (diferenca < -PI) diferenca += 2.0f * PI;
        cam.paraAngulo = cam.angulo + diferenca;

        cam.progresso = 0.0f;
        cam.posicaoAlvo = p;
        cam.direcaoAlvo = d;
    }

    if (cam.progresso < 1.0f) {
        cam.progresso += dt / TEMPO_TRANSICAO;
        if (cam.progresso > 1.0f) cam.progresso = 1.0f;

        // Curva "smoothstep": comeca devagar, acelera e freia no final
        float s = cam.progresso * cam.progresso * (3.0f - 2.0f * cam.progresso);
        cam.x = cam.deX + (cam.paraX - cam.deX) * s;
        cam.z = cam.deZ + (cam.paraZ - cam.deZ) * s;
        cam.angulo = cam.deAngulo + (cam.paraAngulo - cam.deAngulo) * s;
    }
}

// ---------------------------------------------------------------------------
// Escuridao: quanto mais longe da camera, mais escura a cor (sem shaders).
// ---------------------------------------------------------------------------

// Fator de luz entre 0 (preto) e 1 (cor original) para um ponto do chao.
// Usa 1 - (distancia/raio)^2: perto fica bem iluminado e a luz cai rapido
// perto do limite do raio, como uma lanterna.
static float luzEm(float x, float z) {
    float dx = x - cam.x;
    float dz = z - cam.z;
    float d2 = (dx * dx + dz * dz) / (RAIO_VISAO * RAIO_VISAO);
    float luz = 1.0f - d2;
    if (luz < 0.0f) luz = 0.0f;
    return luz;
}

// Multiplica r, g e b pelo fator (0 a 1); a transparencia (a) nao muda
static Color escalarCor(Color cor, float fator) {
    if (fator > 1.0f) fator = 1.0f;
    if (fator < 0.0f) fator = 0.0f;
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

// Quadrilatero (dois triangulos) com a escuridao calculada em cada canto.
// A placa de video faz o degrade entre os cantos, entao uma parede comprida
// vai escurecendo aos poucos em vez de mudar de cor de uma vez.
// Os cantos a, b, c, d vem em sentido anti-horario vistos do lado que
// deve aparecer (o outro lado e descartado pela raylib).
static void desenharQuad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color cor) {
    rlBegin(RL_TRIANGLES);
        vertice(a, cor); vertice(b, cor); vertice(c, cor);
        vertice(a, cor); vertice(c, cor); vertice(d, cor);
    rlEnd();
}

// Fora do mapa conta como parede (nao precisa desenhar a face para fora)
static bool ehParede3D(Mapa *mapa, int linha, int coluna) {
    if (linha < 0 || linha >= mapa->linhas || coluna < 0 || coluna >= mapa->colunas) return true;
    return mapa->matriz[linha][coluna] == '#';
}

// ---------------------------------------------------------------------------
// Cenario: paredes, piso e teto
// ---------------------------------------------------------------------------

// Uma face vertical de parede. "esq" e "dir" sao os cantos de baixo, vistos
// de frente (de fora da parede). sombra < 1 escurece as faces leste/oeste
// para as quinas ficarem visiveis (volume).
static void desenharFaceParede(Vector3 esq, Vector3 dir, float sombra) {
    Color parede = escalarCor(COR_PAREDE_3D, sombra);
    Color rodape = escalarCor(COR_RODAPE_3D, sombra);

    Vector3 esqRodape = { esq.x, ALTURA_RODAPE, esq.z };
    Vector3 dirRodape = { dir.x, ALTURA_RODAPE, dir.z };
    Vector3 esqTopo   = { esq.x, ALTURA_PAREDE, esq.z };
    Vector3 dirTopo   = { dir.x, ALTURA_PAREDE, dir.z };

    desenharQuad(esq, dir, dirRodape, esqRodape, rodape);           // faixa escura embaixo
    desenharQuad(esqRodape, dirRodape, dirTopo, esqTopo, parede);   // resto da parede
}

// '#' vira um cubo, mas so desenhamos as faces que encostam em piso:
// as faces entre duas paredes nunca aparecem, entao nao gastamos tempo nelas.
static void desenharParede3D(Mapa *mapa, int i, int j) {
    float x0 = (float)j, x1 = (float)j + 1.0f;
    float z0 = (float)i, z1 = (float)i + 1.0f;

    if (!ehParede3D(mapa, i + 1, j)) desenharFaceParede((Vector3){ x0, 0, z1 }, (Vector3){ x1, 0, z1 }, 1.0f);  // face sul
    if (!ehParede3D(mapa, i - 1, j)) desenharFaceParede((Vector3){ x1, 0, z0 }, (Vector3){ x0, 0, z0 }, 1.0f);  // face norte
    if (!ehParede3D(mapa, i, j + 1)) desenharFaceParede((Vector3){ x1, 0, z1 }, (Vector3){ x1, 0, z0 }, 0.75f); // face leste
    if (!ehParede3D(mapa, i, j - 1)) desenharFaceParede((Vector3){ x0, 0, z0 }, (Vector3){ x0, 0, z1 }, 0.75f); // face oeste
}

// Piso (virado para cima) e teto (virado para baixo) de uma casa andavel.
// Corredor: cinza com uma placa mais clara no meio. Sala: xadrez marrom.
static void desenharPisoTeto3D(int i, int j, char tipo) {
    float x0 = (float)j, x1 = (float)j + 1.0f;
    float z0 = (float)i, z1 = (float)i + 1.0f;

    if (tipo == '.') {
        desenharQuad((Vector3){ x0, 0, z0 }, (Vector3){ x0, 0, z1 }, (Vector3){ x1, 0, z1 }, (Vector3){ x1, 0, z0 }, COR_CORREDOR_3D);
        // Placa um pouquinho acima do chao (0.005) para nao "brigar" com ele na tela
        float m = 0.08f, y = 0.005f;
        desenharQuad((Vector3){ x0 + m, y, z0 + m }, (Vector3){ x0 + m, y, z1 - m },
                     (Vector3){ x1 - m, y, z1 - m }, (Vector3){ x1 - m, y, z0 + m }, COR_PLACA_3D);
    } else {
        Color cor = ((i + j) % 2 == 0) ? COR_SALA_A_3D : COR_SALA_B_3D;
        desenharQuad((Vector3){ x0, 0, z0 }, (Vector3){ x0, 0, z1 }, (Vector3){ x1, 0, z1 }, (Vector3){ x1, 0, z0 }, cor);
    }

    // Teto: mesma casa, na altura da parede, com os cantos na ordem inversa
    // para a face ficar virada para baixo
    float h = ALTURA_PAREDE;
    desenharQuad((Vector3){ x0, h, z0 }, (Vector3){ x1, h, z0 }, (Vector3){ x1, h, z1 }, (Vector3){ x0, h, z1 }, COR_TETO_3D);
}

// ---------------------------------------------------------------------------
// Objetos: moveis, escada, professor e baterias
// Cada objeto e pequeno, entao usa uma luz so (a do centro da casa).
// ---------------------------------------------------------------------------

// Cubo com a cor escurecida e as arestas marcadas com uma cor mais escura
static void cuboComBorda(Vector3 centro, Vector3 tamanho, Color cor, Color borda, float luz) {
    DrawCubeV(centro, tamanho, escalarCor(cor, luz));
    DrawCubeWires(centro, tamanho.x, tamanho.y, tamanho.z, escalarCor(borda, luz));
}

// Mesa 'M': tampo de madeira e quatro pernas
static void desenharMesa3D(int i, int j) {
    float cx = (float)j + 0.5f, cz = (float)i + 0.5f;
    float luz = luzEm(cx, cz);

    cuboComBorda((Vector3){ cx, 0.43f, cz }, (Vector3){ 0.84f, 0.06f, 0.64f }, COR_MADEIRA_3D, COR_MADEIRA_E_3D, luz);

    // Pernas nos quatro cantos (sinais de dx e dz)
    for (int k = 0; k < 4; k++) {
        float dx = (k % 2 == 0) ? -0.36f : 0.36f;
        float dz = (k < 2) ? -0.26f : 0.26f;
        DrawCube((Vector3){ cx + dx, 0.2f, cz + dz }, 0.06f, 0.4f, 0.06f, escalarCor(COR_MADEIRA_E_3D, luz));
    }
}

// Computador 'C': bloco baixo de metal com um monitor em cima
static void desenharComputador3D(int i, int j) {
    float cx = (float)j + 0.5f, cz = (float)i + 0.5f;
    float luz = luzEm(cx, cz);

    cuboComBorda((Vector3){ cx, 0.22f, cz }, (Vector3){ 0.84f, 0.44f, 0.64f }, COR_METAL_3D, COR_METAL_E_3D, luz);
    DrawCube((Vector3){ cx, 0.47f, cz }, 0.06f, 0.06f, 0.06f, escalarCor(COR_METAL_E_3D, luz)); // pe do monitor
    DrawCube((Vector3){ cx, 0.62f, cz }, 0.5f, 0.32f, 0.08f, escalarCor(COR_METAL_E_3D, luz));  // carcaca

    // A tela e um pouco mais grossa que a carcaca, entao aparece dos dois lados.
    // Ela emite luz: nunca fica mais escura que 50%.
    DrawCube((Vector3){ cx, 0.63f, cz }, 0.42f, 0.24f, 0.1f, escalarCor(COR_TELA_3D, fmaxf(luz, 0.5f)));
}

// Escada: 4 degraus subindo em direcao ao norte (-z). Cinza bloqueada, verde liberada.
static void desenharEscada3D(Mapa *mapa, bool liberada) {
    if (mapa->saida.linha < 0) return;

    float cx = (float)mapa->saida.coluna + 0.5f;
    float cz = (float)mapa->saida.linha + 0.5f;
    float luz = luzEm(cx, cz);
    if (luz <= 0.0f) return;              // longe demais: nao desenha
    if (liberada) luz = fmaxf(luz, 0.6f); // verde liberada brilha no escuro

    Color claro  = liberada ? (Color){ 0, 210, 90, 255 } : (Color){ 135, 135, 140, 255 };
    Color escuro = liberada ? (Color){ 0, 105, 45, 255 } : (Color){  65,  65,  70, 255 };

    for (int k = 0; k < 4; k++) {
        float altura = 0.1f * (float)(k + 1);          // cada degrau e 0.1 mais alto
        float z = cz + 0.375f - (float)k * 0.25f;      // e fica 0.25 mais para tras
        cuboComBorda((Vector3){ cx, altura / 2.0f, z }, (Vector3){ 0.9f, altura, 0.25f },
                     k % 2 == 0 ? claro : escuro, escuro, luz);
    }
}

// Professor: cilindro roxo (cinza depois de ajudado) com cabeca e um "!" amarelo
static void desenharProfessor3D(Professor *professor) {
    if (professor->posicao.linha < 0) return;

    float cx = (float)professor->posicao.coluna + 0.5f;
    float cz = (float)professor->posicao.linha + 0.5f;
    float luz = luzEm(cx, cz);
    if (luz <= 0.0f) return;

    Color cor = escalarCor(professor->ajudado ? GRAY : PURPLE, luz);
    DrawCylinder((Vector3){ cx, 0.0f, cz }, 0.18f, 0.24f, 0.6f, 16, cor); // corpo (mais largo embaixo)
    DrawSphere((Vector3){ cx, 0.72f, cz }, 0.12f, cor);                    // cabeca

    if (!professor->ajudado) {
        // O "!" brilha (minimo 70% de luz) para ser achado de longe
        Color amarelo = escalarCor(YELLOW, fmaxf(luz, 0.7f));
        DrawCube((Vector3){ cx, 0.95f, cz }, 0.04f, 0.08f, 0.04f, amarelo);
        DrawCube((Vector3){ cx, 0.87f, cz }, 0.04f, 0.03f, 0.04f, amarelo);
    }
}

// Baterias: pilha azul flutuando e pulsando, com um brilho transparente em volta
static void desenharBaterias3D(Item *itens) {
    float tempo = (float)GetTime();

    for (Item *atual = itens; atual != NULL; atual = atual->next) {
        float cx = (float)atual->posicao.coluna + 0.5f;
        float cz = (float)atual->posicao.linha + 0.5f;
        float luz = luzEm(cx, cz);
        if (luz <= 0.0f) continue;

        // Brilham: mesmo longe nunca ficam mais escuras que 55%
        float brilho = fmaxf(luz, 0.55f);
        // sin() faz o valor ir e voltar: a bateria sobe/desce e o brilho pulsa.
        // Somar a coluna faz cada bateria balancar num ritmo diferente.
        float y = 0.22f + 0.04f * sinf(tempo * 3.0f + (float)atual->posicao.coluna);
        float pulso = 0.5f + 0.5f * sinf(tempo * 4.0f);

        DrawCylinder((Vector3){ cx, y - 0.11f, cz }, 0.07f, 0.07f, 0.22f, 12, escalarCor(COR_BATERIA_3D, brilho)); // corpo
        DrawCylinder((Vector3){ cx, y + 0.11f, cz }, 0.03f, 0.03f, 0.04f, 8, escalarCor(LIGHTGRAY, brilho));      // polo +
        DrawSphere((Vector3){ cx, y, cz }, 0.2f + 0.04f * pulso, Fade(SKYBLUE, (0.12f + 0.12f * pulso) * brilho)); // brilho
    }
}

// Monta a camera nos olhos do jogador e desenha, nesta ordem: cenario
// (paredes, piso, teto, moveis), escada, professor e baterias
void desenharCena3D(Jogo *jogo) {
    if (jogo == NULL || jogo->mapa.matriz == NULL) return;
    if (!cam.iniciada) render3dReiniciarCamera(jogo);

    Mapa *mapa = &jogo->mapa;

    // Camera nos olhos do jogador, olhando 1 casa para a frente na direcao do angulo
    Camera3D camera = { 0 };
    camera.position = (Vector3){ cam.x, ALTURA_OLHOS, cam.z };
    camera.target   = (Vector3){ cam.x + cosf(cam.angulo), ALTURA_OLHOS, cam.z + sinf(cam.angulo) };
    camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy     = CAMPO_VISAO;
    camera.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(camera);

        // So percorre o quadrado de casas em volta do jogador (raio + 1 de folga)
        int raio = (int)RAIO_VISAO + 1;
        int linhaCam = (int)cam.z, colunaCam = (int)cam.x;

        for (int i = linhaCam - raio; i <= linhaCam + raio; i++) {
            if (i < 0 || i >= mapa->linhas) continue;
            for (int j = colunaCam - raio; j <= colunaCam + raio; j++) {
                if (j < 0 || j >= mapa->colunas) continue;

                // Dentro do quadrado, pula as casas fora do circulo de visao.
                // A folga de 1 casa garante que paredes que cruzam a borda do
                // circulo aparecam ate escurecer por completo.
                float dx = (float)j + 0.5f - cam.x, dz = (float)i + 0.5f - cam.z;
                if (dx * dx + dz * dz > (RAIO_VISAO + 1.0f) * (RAIO_VISAO + 1.0f)) continue;

                char c = mapa->matriz[i][j];
                if (c == '#') {
                    desenharParede3D(mapa, i, j);
                } else {
                    // Moveis ficam em salas, entao o piso embaixo deles e de sala
                    desenharPisoTeto3D(i, j, (c == 'M' || c == 'C') ? ' ' : c);
                    if (c == 'M') desenharMesa3D(i, j);
                    if (c == 'C') desenharComputador3D(i, j);
                }
            }
        }

        desenharEscada3D(mapa, jogo->professor.ajudado);
        desenharProfessor3D(&jogo->professor);
        // Baterias por ultimo: o brilho e transparente e precisa do resto ja desenhado atras dele
        desenharBaterias3D(jogo->itens);

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
    desenhar_item(jogo->itens, origem, t);
    desenhar_professor(&jogo->professor, origem, t);
    desenharSetaJogador2D(&jogo->jogador, origem, t);
}
