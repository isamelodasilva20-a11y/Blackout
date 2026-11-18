#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#include "estrutura.h"
#include "raylib.h"
#include "jogo.h"     // 1. jogo.h primeiro
#include "hud.h"
#include "itens.h"    // 2. itens.h
#include "mapa.h"
#include "professor.h"
#include "ranking.h"


// Cores do HUD
#define COR_HUD_FUNDO   (Color){  10,  12,  20, 220 }
#define COR_HUD_LINHA   (Color){ 255,  96,   2, 255 } // laranja do CESAR (#ff6002)
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

// Lanterna: corpo retangular, lente na direita e facho de luz quando ligada
static void desenharIconeLanterna(float x, float y, float largura, float altura, bool ligada) {
    Color corCorpo = ligada ? YELLOW : DARKGRAY;
    Color corLente = ligada ? WHITE : GRAY;

    // Cabo/corpo da lanterna
    DrawRectangleRounded((Rectangle){ x, y + altura * 0.22f, largura * 0.65f, altura * 0.56f }, 0.3f, 4, corCorpo);
    // Cabeça/lente da lanterna
    DrawRectangle((int)(x + largura * 0.65f), (int)(y + altura * 0.10f), (int)(largura * 0.25f), (int)(altura * 0.80f), corLente);

    // Facho de luz saindo da lente quando ligada
    if (ligada) {
        Vector2 p1 = { x + largura * 0.92f, y + altura * 0.20f };
        Vector2 p2 = { x + largura * 0.92f, y + altura * 0.80f };
        Vector2 p3 = { x + largura * 1.45f, y + altura * 1.05f };
        Vector2 p4 = { x + largura * 1.45f, y - altura * 0.05f };
        DrawTriangle(p1, p2, p3, Fade(YELLOW, 0.45f));
        DrawTriangle(p1, p3, p4, Fade(YELLOW, 0.45f));
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

static void desenharAlertaProximidade(Jogo *jogo) {
    if (jogo == NULL || jogo->robos == NULL) return;

    // Encontra a distancia do robo ativo mais proximo do jogador
    float menorDist = 9999.0f;
    Robo *r = jogo->robos;
    while (r != NULL) {
        if (r->estado != ESTADO_DESLIGADO) {
            float dx = r->posicaoReal.x - (float)jogo->jogador.posicao.coluna;
            float dy = r->posicaoReal.y - (float)jogo->jogador.posicao.linha;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < menorDist) {
                menorDist = dist;
            }
        }
        r = r->next;
    }

    // Ativa o sensor quando algum robo esta a ate 8 casas de distancia
    float raioAlerta = 8.0f;
    if (menorDist > raioAlerta) return;

    // 0.0 (longe) ate 1.0 (colado no jogador)
    float intensidade = 1.0f - (menorDist / raioAlerta);
    if (intensidade < 0.0f) intensidade = 0.0f;
    if (intensidade > 1.0f) intensidade = 1.0f;

    // Frequencia do pisca aumenta conforme a proximidade
    float velocidadePisca = 4.0f + intensidade * 18.0f;
    bool piscaAtivo = (sinf((float)GetTime() * velocidadePisca) > 0.0f);

    Color corAlerta = YELLOW;
    if (menorDist <= 2.5f) {
        corAlerta = RED;
    } else if (menorDist <= 5.0f) {
        corAlerta = ORANGE;
    }

    int larguraBox = 240;
    int alturaBox = 36;
    int posX = (GetScreenWidth() - larguraBox) / 2;
    int posY = 14;

    DrawRectangle(posX, posY, larguraBox, alturaBox, Fade(BLACK, 0.75f));
    DrawRectangleLines(posX, posY, larguraBox, alturaBox, piscaAtivo ? corAlerta : DARKGRAY);

    // Luz circular piscando + texto de alerta
    Color corLuz = piscaAtivo ? corAlerta : Fade(corAlerta, 0.2f);
    DrawCircle(posX + 20, posY + alturaBox / 2, 7.0f, corLuz);

    const char *texto = (menorDist <= 2.5f) ? "PERIGO: ROBO PERTO!" : "ALERTA DE PROXIMIDADE";
    DrawText(texto, posX + 36, posY + 11, 14, piscaAtivo ? corAlerta : LIGHTGRAY);
}

// Barra de baixo com duas linhas:
//   linha 1: vidas, andar, bateria, pendrive, lanterna (esquerda) e cronometro (direita)
//   linha 2: objetivo atual (esquerda) e controles do modo de visao (direita)
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

    // Vidas: desenha apenas os coracoes restantes (mantendo o espaco fixo do HUD)
    float raioCoracao = tamanhoFonte * 0.45f;
    for (int i = 0; i < VIDAS_INICIAIS; i++) {
        if (i < jogo->jogador.vidas) {
            desenharCoracao(x + raioCoracao, meioLinha1, raioCoracao, COR_VIDA);
        }
        x += raioCoracao * 2.0f + 6.0f * escala;
    }
    x += espaco;

    // Andar atual
    const char *textoAndar = TextFormat("ANDAR %d/%d", jogo->andarAtual, TOTAL_ANDARES);
    DrawText(textoAndar, (int)x, linha1, fonte, WHITE);
    x += (float)MeasureText(textoAndar, fonte) + espaco;
    
    // Carga atual da bateria (0 a 100%)
    float progresso = (float)jogo->jogador.bateria / 100.0f;
    if (progresso < 0.0f) progresso = 0.0f;
    if (progresso > 1.0f) progresso = 1.0f;

    float alturaIcone = tamanhoFonte * 0.8f;
    desenharIconeBateria(x, meioLinha1 - alturaIcone / 2.0f, alturaIcone * 1.6f, alturaIcone, progresso, escala);
    x += alturaIcone * 1.6f + alturaIcone * 0.2f + 8.0f * escala;
    const char *textoBaterias = TextFormat("%d%%", jogo->jogador.bateria);
    DrawText(textoBaterias, (int)x, linha1, fonte, WHITE);
    x += (float)MeasureText(textoBaterias, fonte) + espaco;

    // Pendrive: verde quando o jogador ja ganhou do professor, cinza antes
    Color corPendrive = jogo->jogador.temPendrive ? GREEN : GRAY;
    desenharIconePendrive(x, meioLinha1 - alturaIcone * 0.35f, alturaIcone * 1.4f, alturaIcone * 0.7f, corPendrive);
    x += alturaIcone * 1.4f + 8.0f * escala;
    DrawText("PENDRIVE", (int)x, linha1, fonte, corPendrive);
    x += (float)MeasureText("PENDRIVE", fonte) + espaco;

    // Estado da Lanterna (Ligada ao apertar L ou Desligada)
    bool lanternaLigada = (jogo->jogador.tempoLanterna > 0.0f && jogo->jogador.bateria > 0);
    desenharIconeLanterna(x, meioLinha1 - alturaIcone / 2.0f, alturaIcone * 1.5f, alturaIcone, lanternaLigada);
    x += alturaIcone * 1.9f + 6.0f * escala;

    const char *textoLanterna = lanternaLigada
        ? "LIGADA"
        : (jogo->jogador.bateria > 0 ? "DESLIGADA [L]" : "DESLIGADA");
    Color corLanterna = lanternaLigada
        ? YELLOW
        : (jogo->jogador.bateria > 0 ? RAYWHITE : GRAY);
    DrawText(textoLanterna, (int)x, linha1, fonte, corLanterna);

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
        ? "WASD: andar   L: lanterna   V: visão 3D   P: pausar   F11: tela cheia"
        : "W/S: andar   A/D: girar   L: lanterna   V: visão 2D   P: pausar";
    int larguraControles = MeasureText(controles, fontePequena);
    int fimObjetivo = (int)margem + MeasureText(objetivo, fontePequena);
    // So mostra os controles se eles nao forem encostar no objetivo
    if (largura - (int)margem - larguraControles > fimObjetivo + (int)espaco) {
        DrawText(controles, largura - (int)margem - larguraControles, linha2, fontePequena, GRAY);
    }

    // Efeito azul e aviso na tela quando tomar curto-circuito do robo cinza no 3o andar
    if (jogo->jogador.tempoCurtoCircuito > 0.0f) {
        DrawRectangle(0, 0, largura, GetScreenHeight(), Fade(SKYBLUE, 0.22f));

        const char *aviso = "CURTO-CIRCUITO! CONTROLES INVERTIDOS!";
        int larguraAviso = MeasureText(aviso, fonte);
        int posXAviso = (largura - larguraAviso) / 2;
        int posYAviso = (int)(58.0f * escala);

        DrawRectangle(posXAviso - 10, posYAviso - 4, larguraAviso + 20, fonte + 8, Fade(BLACK, 0.75f));
        DrawText(aviso, posXAviso, posYAviso, fonte, SKYBLUE);
    } else if (jogo->jogador.tempoInvulneravel > 2.65f) {
        // Flash vermelho rapido na tela ao tomar dano de um robo
        DrawRectangle(0, 0, largura, GetScreenHeight(), Fade(RED, 0.28f));
    }

    desenharAlertaProximidade(jogo);
}

// Escreve um texto centralizado na horizontal
static void textoCentralizado(const char *texto, int y, int tamanho, Color cor) {
    DrawText(texto, (GetScreenWidth() - MeasureText(texto, tamanho)) / 2, y, tamanho, cor);
}

void desenharTelaVitoria(Jogo *jogo, EntradaRanking ranking[], int totalRanking) {
    if (jogo == NULL) return;

    float escala = hudEscala();
    int topo = (int)(50 * escala);

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.85f));
    textoCentralizado("BACKUP CONCLUÍDO! TODOS OS PENDRIVES FORAM COLETADOS!", topo, fonteEscalada(26, escala), GREEN);
    textoCentralizado(TextFormat("Jogador: %s  |  Seu Tempo: %s", jogo->jogador.nome, formatarTempo(jogo->tempoJogo)),
                      topo + (int)(40 * escala), fonteEscalada(22, escala), WHITE);

    // --- TABELA DE RANKING GERAL ---
    int yRank = topo + (int)(95 * escala);
    textoCentralizado("=== RANKING GERAL (TOP 5) ===", yRank, fonteEscalada(24, escala), COR_HUD_LINHA);
    yRank += (int)(40 * escala);

    int limite = (totalRanking < 5) ? totalRanking : 5;
    for (int i = 0; i < limite; i++) {
        // Destaca se for a linha do jogador atual
        bool ehJogadorAtual = (strcmp(ranking[i].nome, jogo->jogador.nome) == 0 &&
                               fabsf(ranking[i].tempo - jogo->tempoJogo) < 0.05f);
        Color corLinha = ehJogadorAtual ? YELLOW : RAYWHITE;

        const char *linhaRank = TextFormat("%dº  %-18s  %s", i + 1, ranking[i].nome, formatarTempo(ranking[i].tempo));
        textoCentralizado(linhaRank, yRank + i * (int)(32 * escala), fonteEscalada(20, escala), corLinha);
    }

    textoCentralizado("Pressione ESC para sair", GetScreenHeight() - (int)(50 * escala), fonteEscalada(20, escala), LIGHTGRAY);
}

// Tela de pausa: escurece o jogo (que continua desenhado atras, parado) e
// mostra o aviso no meio, com as cores da marca
void desenharTelaPausa(void) {
    float escala = hudEscala();
    int meio = GetScreenHeight() / 2;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f));
    textoCentralizado("PAUSADO", meio - (int)(50 * escala), fonteEscalada(60, escala), COR_HUD_LINHA);
    textoCentralizado("Pressione P para continuar", meio + (int)(25 * escala), fonteEscalada(22, escala), RAYWHITE);
    textoCentralizado("ESC: sair do jogo", meio + (int)(58 * escala), fonteEscalada(16, escala), GRAY);
}

void desenharTelaGameOver(void) {
    float escala = hudEscala();
    int meio = GetScreenHeight() / 2;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.80f));
    textoCentralizado("GAME OVER", meio - (int)(60 * escala), fonteEscalada(60, escala), COR_VIDA);
    textoCentralizado("Os robôs destruíram seus arquivos do TCC!", meio + (int)(10 * escala), fonteEscalada(22, escala), RAYWHITE);
    textoCentralizado("Pressione R para tentar novamente ou ESC para sair", meio + (int)(50 * escala), fonteEscalada(18, escala), LIGHTGRAY);
}
