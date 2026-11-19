#include <math.h>
#include <stdio.h>
#include <string.h>
#include "estrutura.h"
#include "raylib.h"
#include "hud.h"      // hudEscala, textoCentralizado e formatarTempo (hud.c)
#include "ranking.h"
#include "cores.h"    // paleta da marca (a mesma do HUD e das outras telas)

/*
 * Tela inicial: titulo, a historia do jogo e "ENTER para comecar", com a luz
 * do predio falhando (a energia esta caindo: e o blackout).
 *
 * Ela tem o proprio laco de desenho e so devolve o controle quando o jogador
 * aperta ENTER (devolve true: comecar) ou ESC / o X da janela (devolve false:
 * sair), por isso o main.c so precisa de uma chamada antes do laco do jogo.
 *
 * Organizacao da tela (de cima para baixo):
 *   1. titulo BLACKOUT
 *   2. historia no centro, com os professores na esquerda e os robos na direita
 *   3. faixa de baixo: controles (esquerda), nome + ENTER (centro), TOP 5 (direita)
 */

// Fundo dos quadros (o mesmo azul-escuro da barra do HUD). As cores da marca
// (COR_LARANJA, COR_FUNDO...) vem de cores.h
#define COR_QUADRO   (Color){  10,  12,  20, 255 }

#define LETRAS_POR_SEGUNDO 45.0f // velocidade da "maquina de escrever"

// Efeito ao apertar ENTER: a lampada pisca rapido, apaga e a tela escurece
// ate ficar toda preta. O jogo comeca no escuro e "acende" (hud.c), entao a
// passagem de uma tela para a outra parece a energia caindo e voltando.
#define TEMPO_PISCANDO  0.35f // segundos piscando antes de apagar de vez
#define TEMPO_ESCURECER 0.50f // segundos para a tela ir de apagada a toda preta
#define TEMPO_APAGAR    1.00f // duracao total (o resto fica no preto)
#define TOP_RANKING 5            // quantos recordes aparecem no quadro

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

// Controles mostrados no quadro da esquerda: { tecla, o que faz }
static const char *CONTROLES[][2] = {
    { "WASD/setas", "andar" },
    { "L",          "lanterna" },
    { "V",          "visão 2D/3D" },
    { "P",          "pausar" },
    { "F11",        "tela cheia" },
};
#define TOTAL_CONTROLES ((int)(sizeof(CONTROLES) / sizeof(CONTROLES[0])))

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

// Copia o texto para "destino" cortando o final com "..." se ele passar da
// largura maxima (em pixels). Usado nos nomes do ranking, que podem ser longos.
static void encaixarTexto(const char *texto, int larguraMax, int fonte, char *destino, int tamanho) {
    int letras = contarLetras(texto);
    primeirasLetras(texto, letras, destino, tamanho);
    if (MeasureText(destino, fonte) <= larguraMax) return;

    // Tira uma letra por vez ate o texto + "..." caber
    for (letras = letras - 1; letras > 0; letras--) {
        primeirasLetras(texto, letras, destino, tamanho - 3); // reserva espaco para "..."
        strcat(destino, "...");
        if (MeasureText(destino, fonte) <= larguraMax) return;
    }
}

// ---- Personagens -----------------------------------------------------------
// As imagens sao carregadas uma unica vez quando a tela abre e descarregadas
// quando ela fecha (antes de o jogo comecar).
typedef struct {
    Texture2D professores[3]; // os tres professores do jogo (professor_1/2/3_normal.png)
    Texture2D roboVermelho;
    Texture2D roboLaranja;
} Personagens;

static Texture2D carregarImagem(const char *caminho) {
    Texture2D textura = LoadTexture(caminho);
    // As imagens tem ~900 px e aparecem com ~200 px: o filtro bilinear suaviza
    // a reducao (sem ele os pixels "pulam" e a imagem fica serrilhada)
    if (textura.id > 0) SetTextureFilter(textura, TEXTURE_FILTER_BILINEAR);
    return textura;
}

static void personagensCarregar(Personagens *p) {
    p->professores[0] = carregarImagem("imagens/professor_1_normal.png");
    p->professores[1] = carregarImagem("imagens/professor_2_normal.png");
    p->professores[2] = carregarImagem("imagens/professor_3_normal.png");
    p->roboVermelho   = carregarImagem("imagens/robo_vermelho_frente.png");
    p->roboLaranja    = carregarImagem("imagens/robo_laranja_frente.png");
}

static void personagensDescarregar(Personagens *p) {
    for (int i = 0; i < 3; i++) {
        if (p->professores[i].id > 0) UnloadTexture(p->professores[i]);
    }
    if (p->roboVermelho.id > 0) UnloadTexture(p->roboVermelho);
    if (p->roboLaranja.id > 0)  UnloadTexture(p->roboLaranja);
}

// Maior altura em que a imagem cabe: a desejada, ou menos se a largura
// resultante passar de larguraMax (a proporcao da imagem e mantida)
static float alturaQueCabe(Texture2D tex, float alturaDesejada, float larguraMax) {
    if (tex.id == 0 || tex.width == 0) return 0.0f;
    float alturaPelaLargura = larguraMax * (float)tex.height / (float)tex.width;
    return (alturaDesejada < alturaPelaLargura) ? alturaDesejada : alturaPelaLargura;
}

// Desenha um personagem em pe sobre o chao (yPiso), centralizado em centroX:
// primeiro um brilho laranja atras (a luz da lampada batendo nele), depois a
// sombra no chao e por fim a imagem, escurecida pela "tinta".
// Devolve o retangulo da imagem na tela (para desenhar os olhos do robo).
static Rectangle desenharPersonagem(Texture2D tex, float centroX, float yPiso, float altura,
                                    float flutuar, Color tinta, float luz) {
    Rectangle destino = { 0 };
    if (tex.id == 0 || altura <= 0.0f) return destino;

    float largura = altura * (float)tex.width / (float)tex.height;
    destino = (Rectangle){ centroX - largura / 2.0f, yPiso - altura + flutuar, largura, altura };

    DrawCircleGradient((Vector2){ centroX, yPiso - altura * 0.5f }, altura * 0.75f,
                       Fade(COR_LARANJA2, 0.14f * luz), Fade(COR_FUNDO, 0.0f));
    DrawEllipse((int)centroX, (int)yPiso, largura * 0.38f, altura * 0.035f, Fade(BLACK, 0.55f));
    DrawTexturePro(tex, (Rectangle){ 0.0f, 0.0f, (float)tex.width, (float)tex.height }, destino,
                   (Vector2){ 0.0f, 0.0f }, 0.0f, tinta);
    return destino;
}

// Professor na esquerda e dois robos na direita, no espaco livre ao lado da
// historia. Devolve o retangulo do robo vermelho (os olhos dele sao
// desenhados no fim, por cima da penumbra, para brilharem no escuro).
static Rectangle desenharPersonagens(const Personagens *p, float luz, float tempo, int cx,
                                     float meiaHistoria, float yPiso, float e) {
    Rectangle vazio = { 0 };

    // Espaco livre de cada lado da historia; cada grupo fica no meio dele.
    // O espaco e limitado a area da janela base (1024 x escala): num monitor
    // bem largo os personagens nao vao parar no canto da tela, ficam
    // alinhados com os quadros de baixo.
    float bordaBase = (float)cx - (LARGURA_BASE / 2.0f) * e;
    if (bordaBase < 0.0f) bordaBase = 0.0f;
    float lado = (float)cx - meiaHistoria - bordaBase;
    if (lado < 70.0f * e) return vazio; // janela estreita demais: melhor nao cobrir o texto
    float xEsquerda = bordaBase + lado / 2.0f;
    float xDireita = (float)cx + meiaHistoria + lado / 2.0f;

    // Os personagens ficam longe da lampada (que esta no meio do teto): recebem
    // menos luz que o texto e, quando a energia falha, viram quase silhuetas
    float brilho = 0.08f + 0.80f * luz;
    Color tinta = iluminar(WHITE, brilho);

    // Os robos flutuam devagar (seno do tempo), cada um num ritmo
    float flutuaVermelho = sinf(tempo * 2.0f) * 4.0f * e;
    float flutuaLaranja = sinf(tempo * 1.6f + 1.3f) * 4.0f * e;

    // Esquerda: os tres professores em grupo. O professor 2 fica na frente,
    // no centro; o 1 e o 3 ficam atras, um de cada lado, menores, um pouco
    // mais altos e mais escuros (perspectiva: parecem estar mais longe).
    // Os de tras sao desenhados primeiro, para o da frente ficar por cima.
    float altFrente = alturaQueCabe(p->professores[1], 235.0f * e, lado * 0.62f);
    float altFundo = altFrente * 0.78f;
    float afastamento = lado * 0.25f;          // distancia lateral dos de tras
    float yFundo = yPiso - altFrente * 0.16f;  // os de tras "pisam" mais acima
    desenharPersonagem(p->professores[0], xEsquerda - afastamento, yFundo, altFundo, 0.0f,
                       iluminar(tinta, 0.6f), luz * 0.6f);
    desenharPersonagem(p->professores[2], xEsquerda + afastamento, yFundo, altFundo, 0.0f,
                       iluminar(tinta, 0.6f), luz * 0.6f);
    desenharPersonagem(p->professores[1], xEsquerda, yPiso, altFrente, 0.0f, tinta, luz);

    // Direita: o robo vermelho na frente e o laranja atras, menor, mais alto
    // e mais escuro (perspectiva: parece estar mais longe), formando um grupo
    float altVermelho = alturaQueCabe(p->roboVermelho, 180.0f * e, lado * 0.72f);
    float largVermelho = altVermelho * (float)p->roboVermelho.width / (float)(p->roboVermelho.height > 0 ? p->roboVermelho.height : 1);
    float altLaranja = altVermelho * 0.72f;
    desenharPersonagem(p->roboLaranja, xDireita + largVermelho * 0.40f, yPiso - altVermelho * 0.28f, altLaranja,
                       flutuaLaranja, iluminar(tinta, 0.55f), luz * 0.6f);
    return desenharPersonagem(p->roboVermelho, xDireita - largVermelho * 0.12f, yPiso, altVermelho,
                              flutuaVermelho, tinta, luz);
}

// Olhos do robo vermelho: quanto mais escura a sala, mais eles brilham
static void desenharOlhosRobo(Rectangle robo, float luz, float e) {
    if (robo.width <= 0.0f) return;
    float intensidade = 1.0f - luz;
    if (intensidade < 0.15f) return; // com a luz acesa, nao aparece

    // Posicao dos olhos na imagem robo_vermelho_frente.png (em fracao da imagem)
    float olhosX[2] = { 0.43f, 0.567f };
    float olhoY = robo.y + robo.height * 0.305f;
    for (int i = 0; i < 2; i++) {
        Vector2 centro = { robo.x + robo.width * olhosX[i], olhoY };
        DrawCircleGradient(centro, 12.0f * e, Fade(YELLOW, 0.7f * intensidade), Fade(YELLOW, 0.0f));
        DrawCircleV(centro, 2.5f * e, Fade(YELLOW, intensidade));
    }
}

// ---- Quadros da faixa de baixo ---------------------------------------------

// Quadro com fundo escuro translucido, borda fina, filete laranja em cima
// (mesmo estilo da barra do HUD durante o jogo) e titulo
static void desenharQuadro(Rectangle r, const char *titulo, float luz, float e) {
    DrawRectangleRec(r, Fade(COR_QUADRO, 0.8f));
    DrawRectangleLinesEx(r, 1.0f, Fade(COR_CINZA, 0.3f));
    DrawRectangleRec((Rectangle){ r.x, r.y, r.width, (e < 1.0f) ? 1.0f : 2.0f * e },
                     iluminar(COR_LARANJA, 0.3f + 0.7f * luz));
    DrawText(titulo, (int)(r.x + 14.0f * e), (int)(r.y + 12.0f * e), (int)(15.0f * e),
             iluminar(COR_LARANJA, 0.3f + 0.7f * luz));
}

// Quadro da esquerda: cada tecla aparece numa "tecla" desenhada (retangulo
// com borda) e a acao ao lado, sempre na mesma coluna
static void desenharControles(Rectangle r, float luz, float e) {
    desenharQuadro(r, "CONTROLES", luz, e);

    int fonte = (int)(14.0f * e);
    float margem = 14.0f * e;
    float passo = 21.0f * e;
    float y = r.y + 40.0f * e;

    // A coluna das acoes comeca depois da tecla mais larga
    int maiorTecla = 0;
    for (int i = 0; i < TOTAL_CONTROLES; i++) {
        int larg = MeasureText(CONTROLES[i][0], fonte);
        if (larg > maiorTecla) maiorTecla = larg;
    }
    float xAcao = r.x + margem + (float)maiorTecla + 22.0f * e;

    for (int i = 0; i < TOTAL_CONTROLES; i++) {
        float yLinha = y + passo * (float)i;
        int largTecla = MeasureText(CONTROLES[i][0], fonte);
        Rectangle tecla = { r.x + margem, yLinha - 3.0f * e, (float)largTecla + 10.0f * e, (float)fonte + 6.0f * e };
        DrawRectangleLinesEx(tecla, 1.0f, iluminar(COR_CINZA, 0.4f + 0.6f * luz));
        DrawText(CONTROLES[i][0], (int)(tecla.x + 5.0f * e), (int)yLinha, fonte, iluminar(COR_CLARO, 0.35f + 0.65f * luz));
        DrawText(CONTROLES[i][1], (int)xAcao, (int)yLinha, fonte, iluminar(COR_CINZA, 0.35f + 0.65f * luz));
    }
}

// Quadro da direita: os 5 melhores tempos do ranking (lidos uma vez quando a
// tela abre). Posicao na esquerda, tempo alinhado na direita e o nome no
// meio, cortado com "..." se nao couber. O 1o lugar fica em laranja.
static void desenharRanking(Rectangle r, const EntradaRanking lista[], int total, float luz, float e) {
    desenharQuadro(r, "TOP 5 - RECORDES", luz, e);

    int fonte = (int)(15.0f * e);
    float margem = 14.0f * e;

    if (total <= 0) {
        // Arquivo ranking.txt ainda nao existe (ou esta vazio)
        textoCentralizado("Ainda sem recordes", (int)(r.x + r.width / 2.0f), (int)(r.y + r.height / 2.0f), fonte,
                          iluminar(COR_CINZA, 0.35f + 0.65f * luz));
        return;
    }

    int larguraPosicao = MeasureText("5.", fonte) + (int)(8.0f * e);
    int larguraTempo = MeasureText("00:00", fonte);
    int larguraNome = (int)(r.width - 2.0f * margem) - larguraPosicao - larguraTempo - (int)(10.0f * e);

    int limite = (total < TOP_RANKING) ? total : TOP_RANKING;
    for (int i = 0; i < limite; i++) {
        int y = (int)(r.y + 40.0f * e + 21.0f * e * (float)i);
        Color cor = iluminar((i == 0) ? COR_LARANJA : COR_CLARO, 0.35f + 0.65f * luz);

        char nome[64];
        char tempo[16];
        encaixarTexto(lista[i].nome, larguraNome, fonte, nome, (int)sizeof(nome));
        // formatarTempo (hud.c) usa um buffer temporario da raylib: copia na
        // hora, porque o TextFormat logo abaixo poderia reaproveitar o buffer
        snprintf(tempo, sizeof(tempo), "%s", formatarTempo(lista[i].tempo));

        DrawText(TextFormat("%d.", i + 1), (int)(r.x + margem), y, fonte, iluminar(COR_CINZA, 0.35f + 0.65f * luz));
        DrawText(nome, (int)(r.x + margem) + larguraPosicao, y, fonte, cor);
        DrawText(tempo, (int)(r.x + r.width - margem) - MeasureText(tempo, fonte), y, fonte, cor);
    }
}

// ---- Desenho ---------------------------------------------------------------

// "escuridao" vai de 0 (normal) a 1 (tela toda preta): usada no efeito de
// apagar a luz depois do ENTER
static void desenharTela(const Lampada *lampada, float tempo, const char *nomeJogador,
                         const Personagens *personagens, const EntradaRanking ranking[], int totalRanking,
                         float escuridao) {
    int largura = GetScreenWidth(), altura = GetScreenHeight();
    float e = hudEscala();
    float luz = lampada->brilho;
    int cx = largura / 2;

    ClearBackground(COR_FUNDO);

    // Luminaria no teto
    int barra = (int)(260.0f * e);
    DrawCircleGradient((Vector2){ (float)cx, 0.0f }, (float)altura * 0.75f, Fade(COR_LARANJA2, 0.22f * luz), Fade(COR_FUNDO, 0.0f));
    DrawRectangle(cx - barra / 2, (int)(14.0f * e), barra, (int)(6.0f * e), iluminar(COR_CLARO, 0.25f + 0.75f * luz));

    // Titulo
    int fonteTitulo = (int)(80.0f * e);
    int yTitulo = (int)(30.0f * e);
    if (luz < 0.6f) {
        int desvio = GetRandomValue(-6, 6);
        textoCentralizado("BLACKOUT", cx + desvio, yTitulo + GetRandomValue(-2, 2), fonteTitulo, Fade(COR_LARANJA2, 0.6f));
    }
    textoCentralizado("BLACKOUT", cx, yTitulo, fonteTitulo, iluminar(COR_LARANJA, 0.15f + 0.85f * luz));
    textoCentralizado("C E S A R   S C H O O L", cx, yTitulo + fonteTitulo + (int)(4.0f * e), (int)(18.0f * e),
                      iluminar(COR_CINZA, 0.3f + 0.7f * luz));

    // ---- Meio: historia, com os personagens dos lados ----
    // A fonte da historia ficou um pouco menor (18 em vez de 20) para sobrar
    // espaco nas laterais para o professor e os robos
    int fonte = (int)(18.0f * e);
    int espaco = (int)(22.0f * e);
    int yHistoria = yTitulo + fonteTitulo + (int)(44.0f * e);
    float yPiso = (float)(yHistoria + TOTAL_LINHAS * espaco); // "chao" onde os personagens pisam

    // Largura da linha mais longa (a historia completa, nao so o que ja foi
    // digitado): assim os personagens nao mudam de lugar durante a digitacao
    int maiorLinha = 0;
    for (int k = 0; k < TOTAL_LINHAS; k++) {
        int larg = MeasureText(HISTORIA[k], fonte);
        if (larg > maiorLinha) maiorLinha = larg;
    }
    float meiaHistoria = (float)maiorLinha / 2.0f + 20.0f * e; // + uma folga entre texto e personagem

    // Chao: uma linha bem fraca que liga os dois grupos de personagens
    DrawRectangle(0, (int)yPiso, largura, (e < 1.0f) ? 1 : (int)e, Fade(COR_CINZA, 0.10f + 0.10f * luz));
    Rectangle roboVermelho = desenharPersonagens(personagens, luz, tempo, cx, meiaHistoria, yPiso, e);

    // Historia com efeito de maquina de escrever
    int letras = (int)(tempo * LETRAS_POR_SEGUNDO);
    char linha[160];
    for (int k = 0; k < TOTAL_LINHAS && letras > 0; k++) {
        primeirasLetras(HISTORIA[k], letras, linha, (int)sizeof(linha));
        letras -= contarLetras(HISTORIA[k]);
        Color cor = (strstr(HISTORIA[k], "robôs") != NULL || strstr(HISTORIA[k], "hackers para") != NULL) ? COR_LARANJA : COR_CLARO;
        textoCentralizado(linha, cx, yHistoria + k * espaco, fonte, iluminar(cor, 0.35f + 0.65f * luz));
    }

    // ---- Faixa de baixo: controles | nome + ENTER | TOP 5 ----
    // Ancorada no fundo da janela: numa janela mais alta que o normal, o
    // espaco extra fica entre a historia e esta faixa
    float yFaixa = (float)altura - 190.0f * e;
    float alturaQuadro = 158.0f * e;
    float larguraQuadro = 270.0f * e;
    desenharControles((Rectangle){ (float)cx - 490.0f * e, yFaixa, larguraQuadro, alturaQuadro }, luz, e);
    desenharRanking((Rectangle){ (float)cx + 220.0f * e, yFaixa, larguraQuadro, alturaQuadro },
                    ranking, totalRanking, luz, e);

    // --- CAMPO DE NOME DO JOGADOR ---
    // Mais destaque: caixa maior, com um brilho laranja por fora que "respira"
    float pulso = 0.5f + 0.5f * sinf(tempo * 3.0f);
    int largCaixa = (int)(360.0f * e);
    int altCaixa  = (int)(48.0f * e);
    int xCaixa    = cx - largCaixa / 2;
    int yCaixa    = (int)(yFaixa + 26.0f * e);
    float brilhoBorda = 4.0f * e;

    textoCentralizado("DIGITE SEU NOME PARA O RANKING", cx, (int)(yFaixa + 4.0f * e), (int)(15.0f * e),
                      iluminar(COR_CLARO, 0.5f + 0.5f * luz));
    DrawRectangleRec((Rectangle){ (float)xCaixa - brilhoBorda, (float)yCaixa - brilhoBorda,
                                  (float)largCaixa + 2.0f * brilhoBorda, (float)altCaixa + 2.0f * brilhoBorda },
                     Fade(COR_LARANJA, 0.08f + 0.17f * pulso));
    DrawRectangle(xCaixa, yCaixa, largCaixa, altCaixa, (Color){ 35, 38, 48, 255 });
    DrawRectangleLinesEx((Rectangle){ (float)xCaixa, (float)yCaixa, (float)largCaixa, (float)altCaixa },
                         (e < 1.0f) ? 2.0f : 2.0f * e, COR_LARANJA);

    // Se o nome for maior que a caixa, mostra so o final dele (o trecho que
    // esta sendo digitado). O nome so tem letras ASCII (filtro do teclado),
    // entao pular de 1 em 1 byte nao corta nenhuma letra no meio.
    int fonteNome = (int)(24.0f * e);
    const char *visivel = nomeJogador;
    while (*visivel != '\0' && MeasureText(TextFormat("%s_", visivel), fonteNome) > largCaixa - (int)(24.0f * e)) {
        visivel++;
    }

    // Cursor piscando "_"
    const char *cursor = (((int)(tempo * 2.5f)) % 2 == 0) ? "_" : "";
    const char *textoExibido = (strlen(nomeJogador) > 0)
        ? TextFormat("%s%s", visivel, cursor)
        : TextFormat("Seu nome%s", cursor);
    Color corTextoNome = (strlen(nomeJogador) > 0) ? WHITE : COR_CINZA;
    textoCentralizado(textoExibido, cx, yCaixa + (altCaixa - fonteNome) / 2, fonteNome, corTextoNome);

    // "ENTER para comecar" virou um botao laranja cheio, que pulsa de brilho
    float pisca = 0.55f + 0.45f * sinf(tempo * 4.0f);
    Rectangle botao = { (float)xCaixa, yFaixa + 92.0f * e, (float)largCaixa, 42.0f * e };
    DrawRectangleRec((Rectangle){ botao.x - brilhoBorda, botao.y - brilhoBorda,
                                  botao.width + 2.0f * brilhoBorda, botao.height + 2.0f * brilhoBorda },
                     Fade(COR_LARANJA, 0.20f * pisca));
    DrawRectangleRec(botao, iluminar(COR_LARANJA, 0.7f + 0.3f * pisca));
    int fonteBotao = (int)(20.0f * e);
    textoCentralizado("Pressione ENTER para começar", cx, (int)(botao.y + (botao.height - (float)fonteBotao) / 2.0f),
                      fonteBotao, COR_FUNDO);
    textoCentralizado("BACKSPACE apaga uma letra", cx, (int)(botao.y + botao.height + 10.0f * e), (int)(13.0f * e),
                      iluminar(COR_CINZA, 0.4f + 0.6f * luz));

    // Riscos horizontais e penumbra
    for (int ly = 0; ly < altura; ly += 4) DrawRectangle(0, ly, largura, 1, Fade(BLACK, 0.18f));
    DrawRectangle(0, 0, largura, altura, Fade(BLACK, 0.45f * (1.0f - luz)));

    // Por ultimo (por cima da penumbra), os olhos do robo, que brilham no escuro
    desenharOlhosRobo(roboVermelho, luz, e);

    // Efeito de apagar: um retangulo preto por cima de tudo, cada vez mais
    // opaco (os olhos do robo sao a ultima coisa a sumir)
    if (escuridao > 0.0f) DrawRectangle(0, 0, largura, altura, Fade(BLACK, escuridao));
}

// Brilho da lampada durante o efeito de apagar (t = segundos desde o ENTER):
// primeiro alterna rapido entre fraca e forte (a energia caindo), depois apaga
static float brilhoApagando(float t) {
    if (t < TEMPO_PISCANDO) return (((int)(t * 22.0f)) % 2 == 1) ? 0.15f : 0.9f;
    return 0.0f;
}

bool telaInicialMostrar(char *nomeJogador, int maxChars) {
    // Comeca como "sair": so vira true se o jogador apertar ENTER. Assim,
    // fechar pelo X da janela ou pelo ESC devolve false e o main.c vai direto
    // para a limpeza, sem comecar a partida.
    bool comecar = false;
    Lampada lampada = { 1.0f, 0.5f, 0 };
    float tempo = 0.0f;
    int tamanhoAtual = (int)strlen(nomeJogador);

    // Carregados uma vez so, antes do laco (e nao a cada quadro)
    Personagens personagens;
    personagensCarregar(&personagens);
    EntradaRanking ranking[MAX_RANKING];
    int totalRanking = rankingCarregar(ranking, MAX_RANKING); // 0 se ranking.txt nao existir

    // Depois do ENTER a tela nao fecha na hora: roda o efeito de apagar a luz
    bool apagando = false;
    float tempoApagando = 0.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        tempo += dt;

        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();

        if (apagando) {
            // Durante o efeito, o teclado nao digita mais nada: so a luz muda
            tempoApagando += dt;
            lampada.brilho = brilhoApagando(tempoApagando);
            float escuridao = (tempoApagando - TEMPO_PISCANDO) / TEMPO_ESCURECER;
            if (escuridao < 0.0f) escuridao = 0.0f;
            if (escuridao > 1.0f) escuridao = 1.0f;

            BeginDrawing();
                desenharTela(&lampada, tempo, nomeJogador, &personagens, ranking, totalRanking, escuridao);
            EndDrawing();

            // Terminou (a tela ja esta toda preta): agora sim comeca o jogo
            if (tempoApagando >= TEMPO_APAGAR) {
                comecar = true;
                break;
            }
            continue;
        }

        atualizarLampada(&lampada, dt);

        // Captura caracteres digitados (evita ';' para não quebrar o formato do arquivo)
        int tecla = GetCharPressed();
        while (tecla > 0) {
            if ((tecla >= 32) && (tecla <= 125) && (tecla != ';') && (tamanhoAtual < maxChars - 1)) {
                nomeJogador[tamanhoAtual] = (char)tecla;
                nomeJogador[tamanhoAtual + 1] = '\0';
                tamanhoAtual++;
            }
            tecla = GetCharPressed();
        }

        // Apagar com Backspace
        if (IsKeyPressed(KEY_BACKSPACE) && tamanhoAtual > 0) {
            tamanhoAtual--;
            nomeJogador[tamanhoAtual] = '\0';
        }

        // Só inicia com ENTER (espaço agora funciona para digitar nome composto!)
        // ENTER comeca o efeito de apagar a luz (o jogo comeca quando ele terminar)
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            apagando = true;
            tempoApagando = 0.0f;
        }

        // ESC sai do jogo. O fechamento automatico do ESC foi desligado no
        // main.c (SetExitKey), entao a saida e tratada aqui, de forma explicita
        if (IsKeyPressed(KEY_ESCAPE)) break;

        BeginDrawing();
            desenharTela(&lampada, tempo, nomeJogador, &personagens, ranking, totalRanking, 0.0f);
        EndDrawing();
    }

    // As texturas da tela inicial nao sao usadas no jogo: libera aqui mesmo
    personagensDescarregar(&personagens);
    return comecar;
}
