#include <math.h>
#include <string.h>
#include "raylib.h"
#include "hud.h"

/*
 * Tela inicial: titulo, a historia do jogo e "ENTER para comecar", com a luz
 * do predio falhando (a energia esta caindo: e o blackout).
 *
 * Ela tem o proprio laco de desenho e so devolve o controle quando o jogador
 * aperta ENTER/ESPACO (ou fecha a janela), por isso o main.c so precisa de
 * uma chamada antes do laco do jogo.
 */

// Paleta do guia de marca do CESAR
#define COR_FUNDO    (Color){  22,  22,  22, 255 } // #161616
#define COR_LARANJA  (Color){ 255,  96,   2, 255 } // #ff6002
#define COR_LARANJA2 (Color){ 221,  64,  18, 255 } // #dd4012
#define COR_CLARO    (Color){ 255, 254, 238, 255 } // #fffeee
#define COR_CINZA    (Color){ 129, 139, 157, 255 } // #818b9d

#define LETRAS_POR_SEGUNDO 45.0f // velocidade da "maquina de escrever"

// A historia, uma linha de tela por item ("" = linha em branco)
static const char *HISTORIA[] = {
    "A CESAR School foi invadida por hackers.",
    "",
    "Os seus arquivos do TCC sumiram dos servidores, e só os",
    "professores da cadeira de PIF possuem os backups!.",
    "",
    "Encontre seus professores, responda suas perguntas e ",
    "recupere seu projeto antes que seja tarde.",
    "",
    "Cuidado: os robôs do professor Mineiro foram modificados pelos",
    "hackers para pegar você e destruir seus arquivos.",
    "",
    "Ah! E a energia do prédio está falhando...",
};
#define TOTAL_LINHAS ((int)(sizeof(HISTORIA) / sizeof(HISTORIA[0])))

// ---- Luz falhando ----------------------------------------------------------
// A lampada passa a maior parte do tempo acesa, tremendo de leve. De vez em
// quando entra numa "falha": pisca varias vezes bem rapido, as vezes apagando
// quase tudo, e volta.
typedef struct {
    float brilho;      // 0 = apagada, 1 = acesa
    float ateMudar;    // segundos ate sortear o proximo estado
    int piscadas;      // piscadas rapidas que ainda faltam na falha atual
} Lampada;

static float sorteio(float minimo, float maximo) {
    return minimo + (maximo - minimo) * (float)GetRandomValue(0, 1000) / 1000.0f;
}

static void atualizarLampada(Lampada *l, float dt) {
    l->ateMudar -= dt;
    if (l->ateMudar > 0.0f) return;

    if (l->piscadas > 0) {
        // No meio de uma falha: alterna entre quase apagada e acesa, bem rapido
        l->piscadas--;
        l->brilho = (l->piscadas % 2 == 1) ? sorteio(0.05f, 0.35f) : sorteio(0.7f, 1.0f);
        l->ateMudar = sorteio(0.03f, 0.12f);
    } else if (GetRandomValue(0, 100) < 22) {
        // Comeca uma falha nova, com 3 a 9 piscadas
        l->piscadas = GetRandomValue(3, 9);
        l->ateMudar = 0.0f;
    } else {
        // Acesa, com um tremor leve
        l->brilho = sorteio(0.88f, 1.0f);
        l->ateMudar = sorteio(0.08f, 0.5f);
    }
}

// Cor com o brilho da lampada aplicado (escurece r, g e b)
static Color iluminar(Color cor, float brilho) {
    return (Color){ (unsigned char)(cor.r * brilho), (unsigned char)(cor.g * brilho),
                    (unsigned char)(cor.b * brilho), cor.a };
}

// ---- Texto -----------------------------------------------------------------

// Copia para "destino" as primeiras "letras" letras de "texto". Letras com
// acento ocupam 2 bytes em UTF-8: os bytes de continuacao (10xxxxxx) nao
// contam como letra nova, para nunca cortar uma letra no meio.
static void primeirasLetras(const char *texto, int letras, char *destino, int tamanho) {
    int k = 0;
    while (texto[k] != '\0' && k < tamanho - 1) {
        bool comecoDeLetra = ((unsigned char)texto[k] & 0xC0) != 0x80;
        if (comecoDeLetra && letras-- <= 0) break;
        destino[k] = texto[k];
        k++;
    }
    destino[k] = '\0';
}

static int contarLetras(const char *texto) {
    int n = 0;
    for (int k = 0; texto[k] != '\0'; k++) {
        if (((unsigned char)texto[k] & 0xC0) != 0x80) n++;
    }
    return n;
}

static void textoCentralizado(const char *texto, int centroX, int y, int fonte, Color cor) {
    DrawText(texto, centroX - MeasureText(texto, fonte) / 2, y, fonte, cor);
}

// ---- Desenho ---------------------------------------------------------------

static void desenharTela(const Lampada *lampada, float tempo) {
    int largura = GetScreenWidth(), altura = GetScreenHeight();
    float e = hudEscala();
    float luz = lampada->brilho;
    int cx = largura / 2;

    ClearBackground(COR_FUNDO);

    // Luminaria no teto: uma barra e o clarao dela, que some quando a luz falha
    int barra = (int)(260.0f * e);
    DrawCircleGradient((Vector2){ (float)cx, 0.0f }, (float)altura * 0.75f, Fade(COR_LARANJA2, 0.22f * luz), Fade(COR_FUNDO, 0.0f));
    DrawRectangle(cx - barra / 2, (int)(14.0f * e), barra, (int)(6.0f * e), iluminar(COR_CLARO, 0.25f + 0.75f * luz));

    // Titulo. Durante a falha ele "treme": uma copia laranja escura deslocada
    int fonteTitulo = (int)(80.0f * e);
    int yTitulo = (int)(44.0f * e);
    if (luz < 0.6f) {
        int desvio = GetRandomValue(-6, 6);
        textoCentralizado("BLACKOUT", cx + desvio, yTitulo + GetRandomValue(-2, 2), fonteTitulo, Fade(COR_LARANJA2, 0.6f));
    }
    textoCentralizado("BLACKOUT", cx, yTitulo, fonteTitulo, iluminar(COR_LARANJA, 0.15f + 0.85f * luz));
    textoCentralizado("C E S A R   S C H O O L", cx, yTitulo + fonteTitulo + (int)(4.0f * e), (int)(20.0f * e),
                      iluminar(COR_CINZA, 0.3f + 0.7f * luz));

    // Historia, escrita letra por letra (maquina de escrever)
    int fonte = (int)(22.0f * e);
    int espaco = (int)(26.0f * e);
    int y = yTitulo + fonteTitulo + (int)(48.0f * e);
    int letras = (int)(tempo * LETRAS_POR_SEGUNDO);
    char linha[160];
    for (int k = 0; k < TOTAL_LINHAS && letras > 0; k++) {
        primeirasLetras(HISTORIA[k], letras, linha, (int)sizeof(linha));
        letras -= contarLetras(HISTORIA[k]);
        // O aviso dos robos fica em laranja; o resto em claro
        Color cor = (strstr(HISTORIA[k], "robôs") != NULL || strstr(HISTORIA[k], "hackers para") != NULL) ? COR_LARANJA : COR_CLARO;
        textoCentralizado(linha, cx, y + k * espaco, fonte, iluminar(cor, 0.35f + 0.65f * luz));
    }

    // "ENTER para comecar" piscando devagar (nao depende da lampada, para dar para ler)
    float pisca = 0.55f + 0.45f * sinf(tempo * 4.0f);
    textoCentralizado("Pressione ENTER para começar", cx, altura - (int)(86.0f * e), (int)(26.0f * e), Fade(COR_LARANJA, pisca));
    textoCentralizado("WASD ou setas: andar     V: visão 3D     F11: tela cheia", cx, altura - (int)(44.0f * e),
                      (int)(16.0f * e), COR_CINZA);

    // Riscos horizontais bem leves (cara de monitor antigo) e a penumbra da falha
    for (int ly = 0; ly < altura; ly += 4) DrawRectangle(0, ly, largura, 1, Fade(BLACK, 0.18f));
    DrawRectangle(0, 0, largura, altura, Fade(BLACK, 0.45f * (1.0f - luz)));
}

// Mostra a tela inicial ate o jogador apertar ENTER/ESPACO ou fechar a janela
void telaInicialMostrar(void) {
    Lampada lampada = { 1.0f, 0.5f, 0 };
    float tempo = 0.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        tempo += dt;
        atualizarLampada(&lampada, dt);

        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) break;

        BeginDrawing();
            desenharTela(&lampada, tempo);
        EndDrawing();
    }
}
