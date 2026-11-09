#include <stdio.h>
#include <stddef.h>
#include "raylib.h"
#include "jogo.h"     // 1. jogo.h primeiro
#include "hud.h"
#include "itens.h"    // 2. itens.h
#include "mapa.h"
#include "professor.h"

// Cores do HUD
#define COR_HUD_FUNDO   (Color){  10,  12,  20, 220 }
#define COR_HUD_LINHA   (Color){  70, 110, 180, 255 } // mesma cor da luz das paredes
#define COR_VIDA        (Color){ 230,  41,  55, 255 }
#define COR_VIDA_PERDIDA (Color){ 60,  60,  66, 255 }
#define COR_BATERIA_HUD (Color){  40, 120, 255, 255 } // mesmo azul das baterias no 3D

// Quanto a janela atual e maior (ou menor) que a janela base de 1024x640
float hudEscala(void) {
    float escalaX = (float)GetScreenWidth() / LARGURA_BASE;
    float escalaY = (float)GetScreenHeight() / ALTURA_BASE;

    // Usa a menor das duas: assim tudo cabe na tela, tanto numa janela
    // larga e baixa quanto numa estreita e alta
    float escala = (escalaX < escalaY) ? escalaX : escalaY;
    if (escala < 0.5f) escala = 0.5f; // abaixo disso o texto fica ilegivel
    return escala;
}

// A barra do HUD tem 60px na janela base e cresce junto com a escala
int hudAlturaBarra(void) {
    return (int)(HUD_ALTURA_BASE * hudEscala());
}

// Escolhe o tamanho da casa no modo 2D para o mapa ocupar o maximo possivel
// da janela (acima do HUD), sem cortar nada, e centraliza o mapa
void hudLayoutMapa(Mapa *mapa, Vector2 *origem, int *tamanhoCasa) {
    if (mapa == NULL || origem == NULL || tamanhoCasa == NULL || mapa->colunas <= 0 || mapa->linhas <= 0) return;

    // Espaco disponivel: a janela inteira menos a barra do HUD embaixo
    int largura = GetScreenWidth();
    int altura = GetScreenHeight() - hudAlturaBarra();

    // Maior casa (quadrada) que ainda deixa o mapa inteiro caber
    // nas duas direcoes
    int casaPorLargura = largura / mapa->colunas;
    int casaPorAltura = altura / mapa->linhas;
    int casa = (casaPorLargura < casaPorAltura) ? casaPorLargura : casaPorAltura;
    if (casa < 4) casa = 4;

    // Centraliza: a sobra de espaco e dividida igualmente entre os dois lados
    origem->x = (float)((largura - casa * mapa->colunas) / 2);
    origem->y = (float)((altura - casa * mapa->linhas) / 2);
    *tamanhoCasa = casa;
}

// Tamanho de fonte multiplicado pela escala, nunca menor que 10 (minimo legivel)
static int fonteEscalada(float base, float escala) {
    int tamanho = (int)(base * escala);
    return (tamanho < 10) ? 10 : tamanho;
}

// ---------------------------------------------------------------------------
// Icones desenhados so com formas da raylib
// ---------------------------------------------------------------------------

// Coracao = dois circulos lado a lado (parte de cima) + um triangulo
// apontando para baixo. (cx, cy) e o centro e r e o "raio" do coracao.
static void desenharCoracao(float cx, float cy, float r, Color cor) {
    DrawCircleV((Vector2){ cx - r * 0.5f, cy - r * 0.25f }, r * 0.55f, cor);
    DrawCircleV((Vector2){ cx + r * 0.5f, cy - r * 0.25f }, r * 0.55f, cor);
    // A raylib pede os vertices em sentido anti-horario
    DrawTriangle((Vector2){ cx - r * 1.05f, cy - r * 0.1f },
                 (Vector2){ cx, cy + r * 0.95f },
                 (Vector2){ cx + r * 1.05f, cy - r * 0.1f }, cor);
}

// Pilha deitada: contorno, polo + na direita e preenchimento proporcional
// ao progresso (0 = vazia, 1 = cheia)
static void desenharIconeBateria(float x, float y, float largura, float altura, float progresso, float escala) {
    float borda = (escala < 1.0f) ? 1.0f : 2.0f * escala;
    Rectangle corpo = { x, y, largura, altura };
    DrawRectangleLinesEx(corpo, borda, LIGHTGRAY);
    DrawRectangleRec((Rectangle){ x + largura, y + altura * 0.3f, altura * 0.2f, altura * 0.4f }, LIGHTGRAY);

    if (progresso > 0.0f) {
        float interno = largura - 4.0f * borda;
        DrawRectangleRec((Rectangle){ x + 2.0f * borda, y + 2.0f * borda, interno * progresso, altura - 4.0f * borda },
                         COR_BATERIA_HUD);
    }
}

// Pendrive: corpo arredondado + conector metalico na ponta
static void desenharIconePendrive(float x, float y, float largura, float altura, Color cor) {
    DrawRectangleRounded((Rectangle){ x, y, largura * 0.72f, altura }, 0.4f, 6, cor);
    DrawRectangleRec((Rectangle){ x + largura * 0.72f, y + altura * 0.2f, largura * 0.28f, altura * 0.6f }, LIGHTGRAY);
}

// Relogio: anel com dois ponteiros
static void desenharIconeRelogio(float cx, float cy, float r, Color cor) {
    float espessura = r * 0.18f;
    DrawRing((Vector2){ cx, cy }, r - espessura, r, 0.0f, 360.0f, 24, cor);
    DrawLineEx((Vector2){ cx, cy }, (Vector2){ cx, cy - r * 0.6f }, espessura, cor);       // ponteiro dos minutos
    DrawLineEx((Vector2){ cx, cy }, (Vector2){ cx + r * 0.45f, cy }, espessura, cor);      // ponteiro das horas
}

// Converte segundos em "MM:SS"
static const char *formatarTempo(float segundos) {
    int total = (int)segundos;
    return TextFormat("%02d:%02d", total / 60, total % 60);
}

// ---------------------------------------------------------------------------

// Barra de baixo com duas linhas:
//   linha 1: vidas, andar, baterias, pendrive (esquerda) e cronometro (direita)
//   linha 2: objetivo atual (esquerda) e controles do modo de visao (direita)
// Os itens da linha 1 sao colocados um depois do outro: x guarda onde o
// proximo comeca, e MeasureText diz quanto espaco cada texto ocupou.
void desenharHUD(Jogo *jogo) {
    if (jogo == NULL) return;

    float escala = hudEscala();
    int largura = GetScreenWidth();
    int alturaBarra = hudAlturaBarra();
    int topo = GetScreenHeight() - alturaBarra;

    // Todas as medidas abaixo sao as do tamanho base vezes a escala
    int fonte = fonteEscalada(20, escala);
    int fontePequena = fonteEscalada(14, escala);
    float margem = 12.0f * escala;
    float espaco = 22.0f * escala;   // espaco entre um indicador e outro
    int linha1 = topo + (int)(8.0f * escala);
    int linha2 = topo + (int)(36.0f * escala);
    float tamanhoFonte = (float)fonte; // a mesma fonte em float, para medir os icones
    float meioLinha1 = (float)linha1 + tamanhoFonte / 2.0f; // centro vertical da linha 1, para os icones

    // Fundo semitransparente com um filete colorido em cima
    DrawRectangle(0, topo, largura, alturaBarra, COR_HUD_FUNDO);
    DrawRectangle(0, topo, largura, (escala < 1.0f) ? 1 : (int)(2.0f * escala), COR_HUD_LINHA);

    // ---- Linha 1, da esquerda para a direita ----
    float x = margem;

    // Vidas: um coracao por vida inicial; as que ja foram perdidas ficam cinza
    float raioCoracao = tamanhoFonte * 0.45f;
    for (int i = 0; i < VIDAS_INICIAIS; i++) {
        Color cor = (i < jogo->jogador.vidas) ? COR_VIDA : COR_VIDA_PERDIDA;
        desenharCoracao(x + raioCoracao, meioLinha1, raioCoracao, cor);
        x += raioCoracao * 2.0f + 6.0f * escala;
    }
    x += espaco;

    // Andar atual
    const char *textoAndar = TextFormat("ANDAR %d/%d", jogo->andarAtual, TOTAL_ANDARES);
    DrawText(textoAndar, (int)x, linha1, fonte, WHITE);
    x += (float)MeasureText(textoAndar, fonte) + espaco;

    // Baterias coletadas neste andar (total do andar - as que ainda estao no mapa)
    int total = jogo->totalItensAndar;
    int coletadas = total - contar_itens(jogo->itens);
    float progresso = (total > 0) ? (float)coletadas / (float)total : 0.0f;
    float alturaIcone = tamanhoFonte * 0.8f;
    desenharIconeBateria(x, meioLinha1 - alturaIcone / 2.0f, alturaIcone * 1.6f, alturaIcone, progresso, escala);
    x += alturaIcone * 1.6f + alturaIcone * 0.2f + 8.0f * escala;
    const char *textoBaterias = TextFormat("%d/%d", coletadas, total);
    DrawText(textoBaterias, (int)x, linha1, fonte, WHITE);
    x += (float)MeasureText(textoBaterias, fonte) + espaco;

    // Pendrive: verde quando o jogador ja ganhou do professor, cinza antes
    Color corPendrive = jogo->jogador.temPendrive ? GREEN : GRAY;
    desenharIconePendrive(x, meioLinha1 - alturaIcone * 0.35f, alturaIcone * 1.4f, alturaIcone * 0.7f, corPendrive);
    x += alturaIcone * 1.4f + 8.0f * escala;
    DrawText("PENDRIVE", (int)x, linha1, fonte, corPendrive);

    // Cronometro, alinhado na direita
    const char *textoTempo = formatarTempo(jogo->tempoJogo);
    int larguraTempo = MeasureText(textoTempo, fonte);
    int xTempo = largura - (int)margem - larguraTempo;
    DrawText(textoTempo, xTempo, linha1, fonte, WHITE);
    desenharIconeRelogio((float)xTempo - tamanhoFonte * 0.75f, meioLinha1, tamanhoFonte * 0.45f, WHITE);

    // ---- Linha 2: objetivo na esquerda, controles na direita ----
    const char *objetivo;
    Color corObjetivo;
    if (!jogo->professor.ajudado) {
        objetivo = "Encontre o professor (!) e responda o quiz para liberar a escada";
        corObjetivo = YELLOW;
    } else if (jogo->andarAtual < TOTAL_ANDARES) {
        objetivo = "Professor ajudado! Vá até a escada verde para subir";
        corObjetivo = GREEN;
    } else {
        objetivo = "A ESCADA PARA O ÚLTIMO";
        corObjetivo = GREEN;
    }
    DrawText(objetivo, (int)margem, linha2, fontePequena, corObjetivo);

    const char *controles = (jogo->modoVisao == 0)
        ? "WASD: andar   V: visão 3D   F11: tela cheia"
        : "W/S: andar   A/D: girar   V: visão 2D   F11: tela cheia";
    int larguraControles = MeasureText(controles, fontePequena);
    int fimObjetivo = (int)margem + MeasureText(objetivo, fontePequena);
    // So mostra os controles se eles nao forem encostar no objetivo
    if (largura - (int)margem - larguraControles > fimObjetivo + (int)espaco) {
        DrawText(controles, largura - (int)margem - larguraControles, linha2, fontePequena, GRAY);
    }
}

// Escreve um texto centralizado na horizontal
static void textoCentralizado(const char *texto, int y, int tamanho, Color cor) {
    DrawText(texto, (GetScreenWidth() - MeasureText(texto, tamanho)) / 2, y, tamanho, cor);
}

// Tela final: escurece o jogo e mostra a mensagem e o tempo total, centralizados
void desenharTelaVitoria(Jogo *jogo) {
    if (jogo == NULL) return;

    float escala = hudEscala();
    int meio = GetScreenHeight() / 2;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.75f));
    textoCentralizado("BACKUP CONCLUÍDO! TODOS OS PENDRIVES FORAM COLETADOS!", meio - (int)(60 * escala), fonteEscalada(30, escala), GREEN);
    textoCentralizado(TextFormat("Tempo total: %s", formatarTempo(jogo->tempoJogo)), meio - (int)(10 * escala), fonteEscalada(24, escala), WHITE);
    textoCentralizado("Pressione ESC para sair", meio + (int)(35 * escala), fonteEscalada(20, escala), LIGHTGRAY);
}
