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
#include "cores.h"    // paleta da marca (COR_LARANJA, COR_FUNDO...), a mesma da tela inicial


// Cores so do HUD (as da marca vem de cores.h)
#define COR_HUD_FUNDO   (Color){  10,  12,  20, 220 }
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

// Converte segundos em "MM:SS". Usada pelo HUD, pelas telas e pela tela
// inicial (declarada em hud.h). O texto fica num buffer temporario da raylib
// (TextFormat): quem precisar guardar deve copiar logo (snprintf).
const char *formatarTempo(float segundos) {
    int total = (int)segundos;
    return TextFormat("%02d:%02d", total / 60, total % 60);
}

// ---------------------------------------------------------------------------

static void desenharAlertaProximidade(Jogo *jogo) {
    if (jogo == NULL || !jogo->modoVisao || jogo->robos == NULL) return; //so no 3D

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
    DrawRectangle(0, topo, largura, (escala < 1.0f) ? 1 : (int)(2.0f * escala), COR_LARANJA);

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

// Escreve um texto centralizado na horizontal em torno de centroX (para
// centralizar na tela, centroX = GetScreenWidth() / 2). Usada pelas telas do
// hud.c e pela tela inicial (declarada em hud.h).
void textoCentralizado(const char *texto, int centroX, int y, int fonte, Color cor) {
    DrawText(texto, centroX - MeasureText(texto, fonte) / 2, y, fonte, cor);
}

// Efeito de "acender a luz" no comeco da partida: a tela comeca toda preta
// (a tela inicial termina no preto) e a luz liga como uma lampada
// fluorescente: dois piscos curtos e depois vai acendendo ate ficar normal.
// Cada linha da tabela e { ate quantos segundos, brilho nesse trecho }.
static const float ETAPAS_ACENDER[][2] = {
    { 0.25f, 0.00f },  // ainda no escuro
    { 0.32f, 0.70f },  // pisco 1
    { 0.45f, 0.05f },  //   apaga
    { 0.52f, 0.85f },  // pisco 2
    { 0.68f, 0.10f },  //   apaga de novo
};
#define TOTAL_ETAPAS_ACENDER ((int)(sizeof(ETAPAS_ACENDER) / sizeof(ETAPAS_ACENDER[0])))
#define FIM_ACENDER 1.30f  // a partir daqui a luz fica 100% acesa

void desenharLuzAcendendo(float t) {
    if (t >= FIM_ACENDER) return; // efeito acabou: nao desenha nada

    // Procura na tabela o trecho em que o tempo t esta
    float brilho = -1.0f;
    for (int i = 0; i < TOTAL_ETAPAS_ACENDER; i++) {
        if (t < ETAPAS_ACENDER[i][0]) {
            brilho = ETAPAS_ACENDER[i][1];
            break;
        }
    }
    // Depois dos piscos: sobe em linha reta de 30% ate 100%
    if (brilho < 0.0f) {
        float inicio = ETAPAS_ACENDER[TOTAL_ETAPAS_ACENDER - 1][0];
        brilho = 0.3f + 0.7f * (t - inicio) / (FIM_ACENDER - inicio);
    }
    if (brilho > 1.0f) brilho = 1.0f;

    // Pouco brilho = retangulo preto quase opaco por cima do jogo
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 1.0f - brilho));
}

// Botao clicavel (definido mais abaixo, junto com o game over)
static bool botaoClicavel(Rectangle r, const char *tecla, const char *texto, bool principal,
                          float alfa, float pulso, float escala);

AcaoTela desenharTelaVitoria(Jogo *jogo, EntradaRanking ranking[], int totalRanking) {
    if (jogo == NULL) return ACAO_NENHUMA;

    float escala = hudEscala();
    int topo = (int)(50 * escala);
    int cx = GetScreenWidth() / 2;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.85f));
    textoCentralizado("BACKUP CONCLUÍDO! TODOS OS PENDRIVES FORAM COLETADOS!", cx, topo, fonteEscalada(26, escala), GREEN);
    textoCentralizado(TextFormat("Jogador: %s  |  Seu Tempo: %s", jogo->jogador.nome, formatarTempo(jogo->tempoJogo)),
                      cx, topo + (int)(40 * escala), fonteEscalada(22, escala), WHITE);

    // --- TABELA DE RANKING GERAL ---
    int yRank = topo + (int)(95 * escala);
    textoCentralizado("=== RANKING GERAL (TOP 5) ===", cx, yRank, fonteEscalada(24, escala), COR_LARANJA);
    yRank += (int)(40 * escala);

    int limite = (totalRanking < 5) ? totalRanking : 5;
    for (int i = 0; i < limite; i++) {
        // Destaca se for a linha do jogador atual
        bool ehJogadorAtual = (strcmp(ranking[i].nome, jogo->jogador.nome) == 0 &&
                               fabsf(ranking[i].tempo - jogo->tempoJogo) < 0.05f);
        Color corLinha = ehJogadorAtual ? YELLOW : RAYWHITE;

        const char *linhaRank = TextFormat("%dº  %-18s  %s", i + 1, ranking[i].nome, formatarTempo(ranking[i].tempo));
        textoCentralizado(linhaRank, cx, yRank + i * (int)(32 * escala), fonteEscalada(20, escala), corLinha);
    }

    // Botao SAIR (tecla ESC, tratada no main.c, ou clique), centralizado embaixo
    float larguraBotao = 150.0f * escala, alturaBotao = 46.0f * escala;
    Rectangle botaoSair = { ((float)GetScreenWidth() - larguraBotao) / 2.0f,
                            (float)GetScreenHeight() - 50.0f * escala - alturaBotao, larguraBotao, alturaBotao };
    bool clicouSair = botaoClicavel(botaoSair, "ESC", "SAIR", false, 1.0f, 1.0f, escala);
    return clicouSair ? ACAO_SAIR : ACAO_NENHUMA;
}

// ---------------------------------------------------------------------------
// Tela de game over
//
// Linha do tempo da animacao (t = segundos desde que a tela abriu):
//   0.0 s  flash vermelho do "impacto" e o jogo comeca a escurecer
//   0.2 s  o robo vermelho sobe do fundo da tela
//   0.0 s  titulo GAME OVER com falha (glitch) forte ate 1 s, depois em rajadas
//   0.7 s  quadro de resumo aparece, e as linhas surgem uma por vez
//   1.8 s  botoes R e ESC aparecem
// As teclas R e ESC funcionam desde o primeiro quadro (quem trata e o main.c);
// a animacao e so visual.
// ---------------------------------------------------------------------------

// Prende o valor entre 0 e 1 (para as porcentagens das animacoes)
static float limitar01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

// Curva "comeca rapido e freia no final": deixa as entradas mais suaves que
// uma subida em linha reta
static float suavizar(float x) {
    float resto = 1.0f - limitar01(x);
    return 1.0f - resto * resto * resto;
}

// Segundos desde que a tela de game over abriu. O main.c chama a tela a cada
// quadro enquanto o jogo esta em game over; entao, se a ultima chamada foi ha
// mais de meio segundo, e porque a tela acabou de abrir (primeira derrota ou
// nova derrota depois do R) e esse instante vira o "zero" da animacao.
static float tempoDesdeAbertura(void) {
    static double inicio = 0.0;
    static double ultimaChamada = -100.0;
    double agora = GetTime();
    if (agora - ultimaChamada > 0.5) inicio = agora;
    ultimaChamada = agora;
    return (float)(agora - inicio);
}

// Titulo com efeito de falha (glitch): duas copias deslocadas (vermelha e
// laranja escura) tremem atras do texto, e algumas faixas finas do titulo sao
// redesenhadas fora do lugar, como uma tela com defeito. A falha e forte no
// primeiro segundo e depois volta em rajadas curtas a cada 2,5 s.
static void desenharTituloGlitch(int centroX, int y, int fonte, float t, float escala) {
    const char *titulo = "GAME OVER";
    int largura = MeasureText(titulo, fonte);
    int x = centroX - largura / 2;
    float entrada = limitar01(t / 0.5f);
    bool falhando = (t < 1.0f) || (fmodf(t, 2.5f) < 0.2f);
    int desvio = (int)(8.0f * escala);
    if (desvio < 2) desvio = 2;

    if (falhando) {
        DrawText(titulo, x + GetRandomValue(-desvio, desvio), y + GetRandomValue(-desvio / 3, desvio / 3), fonte,
                 Fade(COR_VIDA, 0.8f * entrada));
        DrawText(titulo, x + GetRandomValue(-desvio, desvio), y + GetRandomValue(-desvio / 3, desvio / 3), fonte,
                 Fade(COR_LARANJA2, 0.8f * entrada));
    }
    DrawText(titulo, x, y, fonte, Fade(COR_LARANJA, entrada));

    if (falhando) {
        // Faixas "rasgadas": o recorte (scissor) limita o desenho a uma faixa
        // fina; dentro dela o titulo e redesenhado deslocado para o lado
        for (int i = 0; i < 3; i++) {
            int yFaixa = y + GetRandomValue(0, fonte - 1);
            int alturaFaixa = GetRandomValue(2, (int)(6.0f * escala) + 2);
            int deslocamento = GetRandomValue(-2 * desvio, 2 * desvio);
            DrawRectangle(x - 2 * desvio, yFaixa, largura + 4 * desvio, alturaFaixa, Fade(COR_FUNDO, 0.9f));
            BeginScissorMode(x - 3 * desvio, yFaixa, largura + 6 * desvio, alturaFaixa);
                DrawText(titulo, x + deslocamento, y, fonte, Fade(COR_CLARO, entrada));
            EndScissorMode();
        }
    }
}

// ---- Qual robo aparece no game over ---------------------------------------

// Como o robo "vilao" e desenhado: a imagem de frente, onde ficam os olhos
// (em fracao da imagem, medidos em cada PNG) e as cores dos olhos e do brilho
typedef struct {
    Texture2D textura;
    float olhoX[2];   // x de cada olho (0 = borda esquerda, 1 = direita)
    float olhoY;      // altura dos olhos (0 = topo, 1 = base)
    Color corOlho;
    Color corBrilho;
} VisualVilao;

// As texturas do robo cinza (jogo->texRoboCinza) nao sao carregadas pelo
// jogo, entao a imagem dele e carregada aqui, uma vez so (na primeira vez que
// for preciso), e liberada em hudDescarregar() quando o jogo fecha
static Texture2D texturaCinza = { 0 };
static bool tentouCarregarCinza = false;

static Texture2D texturaRoboCinza(void) {
    if (!tentouCarregarCinza) {
        tentouCarregarCinza = true; // se o arquivo faltar, nao tenta de novo a cada quadro
        texturaCinza = LoadTexture("imagens/robo_cinza_acenando.png");
    }
    return texturaCinza;
}

void hudDescarregar(void) {
    if (texturaCinza.id > 0) UnloadTexture(texturaCinza);
    texturaCinza = (Texture2D){ 0 };
    tentouCarregarCinza = false;
}

// Descobre o robo que causou o game over, sem mexer na logica dos robos:
//  - Se o jogador ainda tem vidas, o jogo acabou porque o robo roxo chegou
//    ao professor antes do quiz: o culpado e o roxo mais perto do professor.
//  - Se as vidas acabaram, o robo que deu o ultimo dano ficou parado colado
//    no jogador (o jogo congela os robos no game over): e o robo ativo mais
//    perto dele, usando os mesmos filtros do dano em checarVitoriaDerrotaRobos
//    (robo fugindo, paralisado, roxo antes do quiz e cinza do 3o andar nao dao dano).
static const Robo *roboCausador(Jogo *jogo, bool *pegouProfessor) {
    bool porProfessor = (jogo->jogador.vidas > 0);
    *pegouProfessor = porProfessor;

    float alvoX = porProfessor ? (float)jogo->professor.posicao.coluna : (float)jogo->jogador.posicao.coluna;
    float alvoY = porProfessor ? (float)jogo->professor.posicao.linha : (float)jogo->jogador.posicao.linha;

    const Robo *melhor = NULL;
    float menorDist = 1e9f;
    for (const Robo *r = jogo->robos; r != NULL; r = r->next) {
        if (r->estado == ESTADO_DESLIGADO) continue;
        if (porProfessor) {
            if (r->tipo != ROBO_TEMPORIZADOR) continue;
        } else {
            if (r->estado == ESTADO_FUGINDO || r->tempoParalisado > 0.0f) continue;
            if (r->tipo == ROBO_TEMPORIZADOR && !jogo->professor.ajudado) continue;
            if (r->tipo == ROBO_CINZA && jogo->andarAtual == 3) continue;
        }
        float dx = r->posicaoReal.x - alvoX;
        float dy = r->posicaoReal.y - alvoY;
        float dist = dx * dx + dy * dy; // distancia ao quadrado: basta para comparar
        if (dist < menorDist) {
            menorDist = dist;
            melhor = r;
        }
    }
    return melhor;
}

// Monta o visual do robo de acordo com o tipo. Usa as imagens de frente que o
// jogo ja carregou (indice [1] = "Baixo (Frente)" em jogo.c); so o cinza usa
// a imagem carregada aqui. Sem robo (nao deveria acontecer), mostra o vermelho.
static VisualVilao visualDoRobo(Jogo *jogo, const Robo *robo) {
    TipoRobo tipo = (robo != NULL) ? robo->tipo : ROBO_PERSEGUIDOR;
    VisualVilao v;

    if (tipo == ROBO_INTERCEPTADOR) {          // laranja (robo_laranja.png)
        v = (VisualVilao){ jogo->texRoboInterceptador[1], { 0.39f, 0.61f }, 0.20f,
                           (Color){ 255, 200, 80, 255 }, COR_LARANJA };
    } else if (tipo == ROBO_TEMPORIZADOR) {    // roxo (robo_transparente.png)
        v = (VisualVilao){ jogo->texRoboTemporizador[1], { 0.36f, 0.64f }, 0.147f,
                           (Color){ 0, 230, 255, 255 }, (Color){ 140, 70, 220, 255 } };
    } else if (tipo == ROBO_CINZA) {           // cinza (robo_cinza_acenando.png)
        v = (VisualVilao){ texturaRoboCinza(), { 0.42f, 0.56f }, 0.33f,
                           (Color){ 90, 200, 255, 255 }, (Color){ 130, 150, 180, 255 } };
    } else {                                   // vermelho (robo_vermelho_frente.png)
        v = (VisualVilao){ jogo->texRoboPerseguidor[1], { 0.43f, 0.567f }, 0.305f,
                           YELLOW, COR_VIDA };
    }
    return v;
}

// O robo que pegou o jogador, "vilao" da tela: sobe do fundo enquanto
// aparece, flutua devagar, tem um brilho na cor dele pulsando atras e os
// olhos acesos.
static void desenharRoboVilao(VisualVilao v, float centroX, float centroY, float altura, float t, float escala) {
    Texture2D tex = v.textura;
    if (tex.id == 0 || tex.height == 0) return;
    float entrada = suavizar((t - 0.2f) / 1.0f);
    if (entrada <= 0.0f) return;

    float largura = altura * (float)tex.width / (float)tex.height;
    float subida = (1.0f - entrada) * 90.0f * escala; // comeca mais baixo e sobe
    float flutua = sinf(t * 1.8f) * 5.0f * escala;
    Rectangle destino = { centroX - largura / 2.0f, centroY - altura / 2.0f + subida + flutua, largura, altura };

    float pulso = 0.5f + 0.5f * sinf(t * 2.5f);
    DrawCircleGradient((Vector2){ centroX, centroY }, altura * 0.7f,
                       Fade(v.corBrilho, (0.16f + 0.12f * pulso) * entrada), Fade(v.corBrilho, 0.0f));
    DrawTexturePro(tex, (Rectangle){ 0.0f, 0.0f, (float)tex.width, (float)tex.height }, destino,
                   (Vector2){ 0.0f, 0.0f }, 0.0f, Fade(WHITE, entrada));

    // Olhos acesos, na posicao medida na imagem de cada robo
    for (int i = 0; i < 2; i++) {
        Vector2 olho = { destino.x + destino.width * v.olhoX[i], destino.y + destino.height * v.olhoY };
        DrawCircleGradient(olho, 16.0f * escala, Fade(v.corOlho, (0.35f + 0.35f * pulso) * entrada), Fade(v.corOlho, 0.0f));
    }
}

// Escada: tres degraus, cada um mais alto (icone do "andar alcancado")
static void desenharIconeEscada(float x, float y, float tamanho, Color cor) {
    float degrau = tamanho / 3.0f;
    for (int i = 0; i < 3; i++) {
        float alturaDegrau = degrau * (float)(i + 1);
        DrawRectangleRec((Rectangle){ x + degrau * (float)i, y + tamanho - alturaDegrau, degrau * 0.85f, alturaDegrau }, cor);
    }
}

// Quadro "RESUMO DA PARTIDA". Cada linha tem icone, nome do dado (cinza) e
// valor (claro, alinhado na direita), e as linhas aparecem uma de cada vez.
static void desenharResumo(Jogo *jogo, Rectangle r, float t, float escala) {
    float entrada = limitar01((t - 0.7f) / 0.4f);
    if (entrada <= 0.0f) return;

    // Pendrives: cada andar so libera a escada depois que o professor entrega
    // o pendrive, entao quem esta no andar N ja pegou N-1, mais 1 se ja pegou
    // o deste andar
    int pendrives = (jogo->andarAtual - 1) + (jogo->jogador.temPendrive ? 1 : 0);
    if (pendrives < 0) pendrives = 0;
    if (pendrives > TOTAL_ANDARES) pendrives = TOTAL_ANDARES;

    DrawRectangleRec(r, Fade(COR_HUD_FUNDO, 0.9f * entrada));
    DrawRectangleLinesEx(r, 1.0f, Fade(COR_CINZA, 0.35f * entrada));
    DrawRectangleRec((Rectangle){ r.x, r.y, r.width, (escala < 1.0f) ? 1.0f : 2.0f * escala }, Fade(COR_LARANJA, entrada));

    float margem = 16.0f * escala;
    int fonteTitulo = fonteEscalada(15, escala);
    int fonte = fonteEscalada(19, escala);
    DrawText("RESUMO DA PARTIDA", (int)(r.x + margem), (int)(r.y + 12.0f * escala), fonteTitulo, Fade(COR_LARANJA, entrada));

    const char *nome = (jogo->jogador.nome[0] != '\0') ? jogo->jogador.nome : "Anonimo";
    const char *rotulos[4] = { "Jogador", "Andar alcançado", "Tempo de jogo", "Pendrives obtidos" };
    char valores[4][64];
    snprintf(valores[0], sizeof(valores[0]), "%s", nome);
    snprintf(valores[1], sizeof(valores[1]), "%d/%d", jogo->andarAtual, TOTAL_ANDARES);
    snprintf(valores[2], sizeof(valores[2]), "%s", formatarTempo(jogo->tempoJogo));
    snprintf(valores[3], sizeof(valores[3]), "%d/%d", pendrives, TOTAL_ANDARES);

    float tamanhoIcone = (float)fonte * 0.9f;
    float xTexto = r.x + margem + tamanhoIcone + 12.0f * escala;
    float larguraValorMax = r.x + r.width - margem - xTexto - 150.0f * escala; // espaco que sobra para o nome

    for (int i = 0; i < 4; i++) {
        // Linha i aparece 0,2 s depois da anterior
        float alfa = limitar01((t - 0.9f - 0.2f * (float)i) / 0.25f);
        if (alfa <= 0.0f) continue;

        float y = r.y + 42.0f * escala + 32.0f * escala * (float)i;
        float meio = y + (float)fonte / 2.0f;
        float xIcone = r.x + margem;

        if (i == 0) {
            desenharCoracao(xIcone + tamanhoIcone / 2.0f, meio, tamanhoIcone * 0.45f, Fade(COR_VIDA_PERDIDA, alfa));
        } else if (i == 1) {
            desenharIconeEscada(xIcone, meio - tamanhoIcone / 2.0f, tamanhoIcone, Fade(COR_LARANJA, alfa));
        } else if (i == 2) {
            desenharIconeRelogio(xIcone + tamanhoIcone / 2.0f, meio, tamanhoIcone * 0.5f, Fade(COR_CLARO, alfa));
        } else {
            desenharIconePendrive(xIcone, meio - tamanhoIcone * 0.3f, tamanhoIcone, tamanhoIcone * 0.6f,
                                  Fade(pendrives > 0 ? GREEN : GRAY, alfa));
        }

        DrawText(rotulos[i], (int)xTexto, (int)y, fonte, Fade(COR_CINZA, alfa));

        // Nome muito comprido: corta o final ate caber (os nomes sao ASCII,
        // filtrados na tela inicial, entao cortar byte a byte e seguro)
        if (i == 0) {
            int tamanho = (int)strlen(valores[0]);
            while (tamanho > 1 && MeasureText(valores[0], fonte) > (int)larguraValorMax) {
                valores[0][--tamanho] = '\0';
            }
        }
        int larguraValor = MeasureText(valores[i], fonte);
        DrawText(valores[i], (int)(r.x + r.width - margem) - larguraValor, (int)y, fonte, Fade(COR_CLARO, alfa));
    }
}

// Botao com a tecla desenhada numa "tecla" (retangulo com borda) e o texto ao
// lado. O principal (R) e laranja cheio e pulsa; o secundario (ESC) so tem borda.
static void desenharBotaoTecla(Rectangle r, const char *tecla, const char *texto, bool principal,
                               float alfa, float pulso, float escala) {
    int fonte = fonteEscalada(17, escala);
    Color corTexto = principal ? COR_FUNDO : COR_CLARO;

    if (principal) {
        float brilho = 4.0f * escala;
        DrawRectangleRec((Rectangle){ r.x - brilho, r.y - brilho, r.width + 2.0f * brilho, r.height + 2.0f * brilho },
                         Fade(COR_LARANJA, 0.25f * pulso * alfa));
        DrawRectangleRec(r, Fade(COR_LARANJA, alfa));
    } else {
        DrawRectangleRec(r, Fade(COR_HUD_FUNDO, 0.9f * alfa));
        DrawRectangleLinesEx(r, (escala < 1.0f) ? 1.0f : 2.0f * escala, Fade(COR_CINZA, alfa));
    }

    // "Tecla" com a letra, na esquerda do botao
    int larguraLetra = MeasureText(tecla, fonte);
    Rectangle capa = { r.x + 10.0f * escala, r.y + (r.height - (float)fonte - 10.0f * escala) / 2.0f,
                       (float)larguraLetra + 14.0f * escala, (float)fonte + 10.0f * escala };
    DrawRectangleLinesEx(capa, (escala < 1.0f) ? 1.0f : 2.0f * escala, Fade(corTexto, alfa));
    DrawText(tecla, (int)(capa.x + 7.0f * escala), (int)(capa.y + 5.0f * escala), fonte, Fade(corTexto, alfa));

    // Texto centralizado no espaco que sobra a direita da tecla
    float inicioTexto = capa.x + capa.width;
    float espacoTexto = r.x + r.width - inicioTexto;
    DrawText(texto, (int)(inicioTexto + (espacoTexto - (float)MeasureText(texto, fonte)) / 2.0f),
             (int)(r.y + (r.height - (float)fonte) / 2.0f), fonte, Fade(corTexto, alfa));
}

// Desenha o botao e diz se ele foi clicado neste quadro.
// O clique vale quando o botao esquerdo do mouse e apertado com a seta dentro
// do retangulo do botao (CheckCollisionPointRec). Com a seta em cima, o botao
// fica "aceso" (brilho cheio e contorno claro) para mostrar que da para clicar.
static bool botaoClicavel(Rectangle r, const char *tecla, const char *texto, bool principal,
                          float alfa, float pulso, float escala) {
    bool mouseEmCima = CheckCollisionPointRec(GetMousePosition(), r);
    desenharBotaoTecla(r, tecla, texto, principal, alfa, mouseEmCima ? 1.0f : pulso, escala);
    if (mouseEmCima) {
        float folga = 3.0f * escala;
        DrawRectangleLinesEx((Rectangle){ r.x - folga, r.y - folga, r.width + 2.0f * folga, r.height + 2.0f * folga },
                             (escala < 1.0f) ? 1.0f : 1.5f * escala, Fade(COR_CLARO, 0.8f * alfa));
    }
    return mouseEmCima && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

AcaoTela desenharTelaGameOver(Jogo *jogo) {
    if (jogo == NULL) return ACAO_NENHUMA;
    AcaoTela acao = ACAO_NENHUMA;

    float escala = hudEscala();
    int largura = GetScreenWidth();
    int altura = GetScreenHeight();
    float t = tempoDesdeAbertura();

    // Tudo e posicionado a partir do centro da janela e em medidas "base"
    // vezes a escala, para ficar igual na janela normal e em tela cheia.
    // Coluna de textos um pouco para a esquerda, robo na direita.
    float meioX = (float)largura / 2.0f;
    float meioY = (float)altura / 2.0f;
    int colunaX = (int)(meioX - 140.0f * escala);

    // 1. Escurecimento gradual do jogo (que continua desenhado atras, parado)
    //    e um flash vermelho rapido no comeco, como um "impacto"
    DrawRectangle(0, 0, largura, altura, Fade(BLACK, 0.9f * suavizar(t / 0.8f)));
    if (t < 0.4f) DrawRectangle(0, 0, largura, altura, Fade(COR_VIDA, 0.35f * (1.0f - t / 0.4f)));

    // 2. O robo que causou o game over, grande, na direita
    bool pegouProfessor = false;
    const Robo *causador = roboCausador(jogo, &pegouProfessor);
    desenharRoboVilao(visualDoRobo(jogo, causador), meioX + 290.0f * escala, meioY, 360.0f * escala, t, escala);

    // 3. Titulo com glitch e a frase de derrota (diferente quando o robo roxo
    //    chegou ao professor antes do jogador)
    desenharTituloGlitch(colunaX, (int)(meioY - 182.0f * escala), fonteEscalada(72, escala), t, escala);
    float entradaFrase = limitar01((t - 0.4f) / 0.4f);
    const char *frase = pegouProfessor ? "O robô roxo chegou ao professor antes de você!"
                                       : "Os robôs destruíram seus arquivos do TCC!";
    DrawText(frase, colunaX - MeasureText(frase, fonteEscalada(18, escala)) / 2,
             (int)(meioY - 98.0f * escala), fonteEscalada(18, escala), Fade(COR_CLARO, entradaFrase));

    // 4. Resumo da partida
    float larguraColuna = 400.0f * escala;
    Rectangle quadro = { (float)colunaX - larguraColuna / 2.0f, meioY - 60.0f * escala, larguraColuna, 172.0f * escala };
    desenharResumo(jogo, quadro, t, escala);

    // 5. Botoes R e ESC, centralizados embaixo do resumo. Eles so aceitam
    //    clique depois que aparecem (as teclas funcionam desde o inicio)
    float alfaBotoes = limitar01((t - 1.8f) / 0.4f);
    if (alfaBotoes > 0.0f) {
        float pulso = 0.5f + 0.5f * sinf(t * 4.0f);
        float larguraR = 230.0f * escala, larguraEsc = 150.0f * escala, vao = 20.0f * escala;
        float xInicio = (float)colunaX - (larguraR + vao + larguraEsc) / 2.0f;
        float yBotoes = meioY + 132.0f * escala;
        float alturaBotao = 46.0f * escala;
        if (botaoClicavel((Rectangle){ xInicio, yBotoes, larguraR, alturaBotao }, "R", "TENTAR DE NOVO",
                          true, alfaBotoes, pulso, escala)) {
            acao = ACAO_REINICIAR;
        }
        // ESC / MENU volta para a tela inicial (quem abre o menu e o main.c)
        if (botaoClicavel((Rectangle){ xInicio + larguraR + vao, yBotoes, larguraEsc, alturaBotao }, "ESC", "MENU",
                          false, alfaBotoes, pulso, escala)) {
            acao = ACAO_MENU;
        }
    }

    // 6. Riscos horizontais por cima de tudo (a mesma "tela de TV" da tela inicial)
    float alfaRiscos = 0.18f * limitar01(t / 0.8f);
    for (int ly = 0; ly < altura; ly += 4) DrawRectangle(0, ly, largura, 1, Fade(BLACK, alfaRiscos));
    return acao;
}

// ---------------------------------------------------------------------------
// Tela de pausa (issue #8)
//
// Mesmo estilo do game over, mas sem animacao de entrada (a pausa precisa
// aparecer na hora): titulo laranja com sombra, quadro com o resumo da
// partida, botoes CONTINUAR e SAIR e um lembrete discreto dos controles.
// Nada e carregado aqui: tudo e desenhado com formas e texto.
// ---------------------------------------------------------------------------

// Vidas como coracoes: cheios (vermelhos) para as que restam e apagados
// (cinza) para as perdidas. Desenha da direita para a esquerda a partir de
// xDireita, para alinhar com os outros valores do quadro.
static void desenharVidasResumo(float xDireita, float meio, int vidas, float tamanho, float escala) {
    float raio = tamanho * 0.45f;
    float passo = raio * 2.0f + 6.0f * escala;
    for (int i = VIDAS_INICIAIS - 1; i >= 0; i--) {
        float cx = xDireita - raio - passo * (float)(VIDAS_INICIAIS - 1 - i);
        desenharCoracao(cx, meio, raio, (i < vidas) ? COR_VIDA : COR_VIDA_PERDIDA);
    }
}

AcaoTela desenharTelaPausa(Jogo *jogo) {
    if (jogo == NULL) return ACAO_NENHUMA;
    AcaoTela acao = ACAO_NENHUMA;

    float escala = hudEscala();
    int largura = GetScreenWidth();
    int altura = GetScreenHeight();
    int cx = largura / 2;
    float meioY = (float)altura / 2.0f;
    float pulso = 0.5f + 0.5f * sinf((float)GetTime() * 4.0f);

    // Fundo: o jogo continua desenhado atras, parado, so que escurecido
    DrawRectangle(0, 0, largura, altura, Fade(BLACK, 0.78f));

    // 1. Titulo com uma "sombra" laranja escura deslocada (mesma familia
    //    de cores do glitch do game over, mas parada)
    int fonteTitulo = fonteEscalada(64, escala);
    int yTitulo = (int)(meioY - 206.0f * escala);
    int sombra = (int)(4.0f * escala);
    int xTitulo = cx - MeasureText("PAUSADO", fonteTitulo) / 2;
    DrawText("PAUSADO", xTitulo + sombra, yTitulo + sombra, fonteTitulo, COR_LARANJA2);
    DrawText("PAUSADO", xTitulo, yTitulo, fonteTitulo, COR_LARANJA);
    textoCentralizado("O cronômetro e os robôs estão parados", cx, (int)(meioY - 132.0f * escala), fonteEscalada(16, escala), COR_CINZA);

    // 2. Quadro com o resumo da partida: icone, nome do dado e valor
    Rectangle r = { (float)cx - 200.0f * escala, meioY - 104.0f * escala, 400.0f * escala, 206.0f * escala };
    DrawRectangleRec(r, Fade(COR_HUD_FUNDO, 0.92f));
    DrawRectangleLinesEx(r, 1.0f, Fade(COR_CINZA, 0.35f));
    DrawRectangleRec((Rectangle){ r.x, r.y, r.width, (escala < 1.0f) ? 1.0f : 2.0f * escala }, COR_LARANJA);

    float margem = 16.0f * escala;
    int fonte = fonteEscalada(19, escala);
    DrawText("RESUMO DA PARTIDA", (int)(r.x + margem), (int)(r.y + 12.0f * escala), fonteEscalada(15, escala), COR_LARANJA);

    float tamanhoIcone = (float)fonte * 0.9f;
    float xIcone = r.x + margem;
    float xTexto = xIcone + tamanhoIcone + 12.0f * escala;
    float xDireita = r.x + r.width - margem; // onde os valores terminam (alinhados na direita)

    // Bateria entre 0 e 100% (a mesma conta do HUD)
    float carga = (float)jogo->jogador.bateria / 100.0f;
    if (carga < 0.0f) carga = 0.0f;
    if (carga > 1.0f) carga = 1.0f;

    const char *rotulos[5] = { "Andar", "Tempo de jogo", "Vidas", "Bateria", "Pendrive do andar" };
    char valores[5][32];
    snprintf(valores[0], sizeof(valores[0]), "%d/%d", jogo->andarAtual, TOTAL_ANDARES);
    snprintf(valores[1], sizeof(valores[1]), "%s", formatarTempo(jogo->tempoJogo));
    valores[2][0] = '\0'; // vidas sao desenhadas como coracoes, sem texto
    snprintf(valores[3], sizeof(valores[3]), "%d%%", jogo->jogador.bateria);
    snprintf(valores[4], sizeof(valores[4]), "%s", jogo->jogador.temPendrive ? "COLETADO" : "FALTA");

    for (int i = 0; i < 5; i++) {
        float y = r.y + 42.0f * escala + 32.0f * escala * (float)i;
        float meio = y + (float)fonte / 2.0f;

        // Icone de cada linha (os mesmos desenhos do HUD e do game over)
        if (i == 0) {
            desenharIconeEscada(xIcone, meio - tamanhoIcone / 2.0f, tamanhoIcone, COR_LARANJA);
        } else if (i == 1) {
            desenharIconeRelogio(xIcone + tamanhoIcone / 2.0f, meio, tamanhoIcone * 0.5f, COR_CLARO);
        } else if (i == 2) {
            desenharCoracao(xIcone + tamanhoIcone / 2.0f, meio, tamanhoIcone * 0.45f, COR_VIDA);
        } else if (i == 3) {
            desenharIconeBateria(xIcone, meio - tamanhoIcone * 0.3f, tamanhoIcone, tamanhoIcone * 0.6f, carga, escala);
        } else {
            desenharIconePendrive(xIcone, meio - tamanhoIcone * 0.3f, tamanhoIcone, tamanhoIcone * 0.6f,
                                  jogo->jogador.temPendrive ? GREEN : GRAY);
        }

        DrawText(rotulos[i], (int)xTexto, (int)y, fonte, COR_CINZA);

        if (i == 2) {
            desenharVidasResumo(xDireita, meio, jogo->jogador.vidas, tamanhoIcone, escala);
        } else {
            Color corValor = (i == 4) ? (jogo->jogador.temPendrive ? GREEN : COR_CLARO) : COR_CLARO;
            DrawText(valores[i], (int)xDireita - MeasureText(valores[i], fonte), (int)y, fonte, corValor);
        }
    }

    // 3. Botoes: CONTINUAR (P ou ESC) e SAIR (Q). As teclas sao lidas no
    //    main.c; aqui so o clique
    float larguraContinuar = 230.0f * escala, larguraSair = 150.0f * escala, vao = 20.0f * escala;
    float xInicio = (float)cx - (larguraContinuar + vao + larguraSair) / 2.0f;
    float yBotoes = meioY + 120.0f * escala;
    float alturaBotao = 46.0f * escala;
    if (botaoClicavel((Rectangle){ xInicio, yBotoes, larguraContinuar, alturaBotao }, "P/ESC", "CONTINUAR",
                      true, 1.0f, pulso, escala)) {
        acao = ACAO_CONTINUAR;
    }
    if (botaoClicavel((Rectangle){ xInicio + larguraContinuar + vao, yBotoes, larguraSair, alturaBotao }, "Q", "SAIR",
                      false, 1.0f, pulso, escala)) {
        acao = ACAO_SAIR;
    }

    // 4. Lembrete dos controles, discreto, no rodape do bloco
    const char *controles = (jogo->modoVisao == 0)
        ? "WASD/setas: andar    V: visão 3D    L: lanterna    F11: tela cheia"
        : "W/S: andar    A/D: girar    V: visão 2D    L: lanterna    F11: tela cheia";
    textoCentralizado(controles, cx, (int)(yBotoes + alturaBotao + 22.0f * escala), fonteEscalada(14, escala), Fade(COR_CINZA, 0.8f));

    // Riscos horizontais (mesma "tela de TV" das outras telas)
    for (int ly = 0; ly < altura; ly += 4) DrawRectangle(0, ly, largura, 1, Fade(BLACK, 0.18f));
    return acao;
}
