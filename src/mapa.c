#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include "raylib.h"
#include "estrutura.h"

#include "robos.h"
#include "mapa.h"
#include "memoria.h"
#include "itens.h"
#include "professor.h"

#define TAM_BUFFER 256
#define DETALHE_MINIMO 12

static Color misturarCor(Color a, Color b, float fator);
static void desenharParede2D(Mapa *mapa, int i, int j, int x, int y, int t);
static void desenharPiso2D(Mapa *mapa, int i, int j, int x, int y, int t, char tipo);
static void desenharMesa2D(int x, int y, int t);
static void desenharComputador2D(int x, int y, int t);
static void desenharLogoTapete2D(int x, int y, int largura, int altura);

// Cores do mapa 2D: paleta do guia de marca do CESAR
// (#ff6002 laranja, #dd4012 laranja escuro, #161616, #3f3f3f, #818b9d, #fffeee)
#define COR_CESAR_LARANJA  (Color){ 255,  96,   2, 255 } // #ff6002
#define COR_CESAR_LARANJA2 (Color){ 221,  64,  18, 255 } // #dd4012
#define COR_CESAR_CLARO    (Color){ 255, 254, 238, 255 } // #fffeee
#define COR_PAREDE_TOPO    (Color){ 129, 139, 157, 255 } // #818b9d: "tampa" da parede vista de cima
#define COR_PAREDE_LUZ     (Color){ 176, 184, 198, 255 } // borda de cima/esquerda (luz)
#define COR_PAREDE_SOMBRA  (Color){  90,  97, 110, 255 } // borda da direita (sombra)
#define COR_CORREDOR       (Color){  30,  30,  30, 255 } // quase #161616
#define COR_CORREDOR_LINHA (Color){  63,  63,  63, 255 } // #3f3f3f: rejunte entre as placas
#define COR_SALA_A         COR_CESAR_CLARO               // piso de sala em xadrez (duas cores)
#define COR_SALA_B         (Color){ 238, 236, 216, 255 }
#define COR_MADEIRA        (Color){  63,  63,  63, 255 } // mesas em grafite (#3f3f3f)
#define COR_MADEIRA_ESCURA (Color){  22,  22,  22, 255 } // #161616
#define COR_METAL          (Color){  63,  63,  63, 255 }
#define COR_METAL_ESCURO   (Color){  22,  22,  22, 255 }
#define COR_TELA           COR_CESAR_LARANJA             // telas acesas em laranja


int mapaCarregar(Jogo *jogo, const char *arquivo) {
    if (jogo == NULL || arquivo == NULL) return 0;

    FILE *f = fopen(arquivo, "r");
    if (f == NULL) {
        printf("Erro: arquivo do mapa não encontrado!\n");
        return 0;
    }
    
    int linhas, colunas;
    if (fscanf(f, "%d %d", &linhas, &colunas) != 2 || linhas <= 0 || colunas <= 0 || colunas > TAM_BUFFER - 2) {
        printf("Erro: cabecalho invalido em %s\n", arquivo);
        fclose(f);
        return 0;
    }

    // Descarta o resto da linha do cabecalho (a quebra de linha)
    char buffer[TAM_BUFFER];
    fgets(buffer, sizeof(buffer), f);

    // 2) Aloca a matriz: um vetor de ponteiros, um para cada linha do mapa
    Mapa *mapa = &jogo->mapa;
    mapa->linhas = linhas;
    mapa->colunas = colunas;
    mapa->saida = (Posicao){ .linha = -1, .coluna = -1 }; // -1 = andar sem escada, ate achar um 'S'
    jogo->professor.posicao = (Posicao){ .linha = -1, .coluna = -1 }; // -1 = ainda nao achou o 'F'
    mapa->matriz = memAlocar((size_t)linhas * sizeof(char *));
    if (mapa->matriz == NULL) {
        fclose(f);
        return 0;
    }

    //lê todo o mapa
    for (int i = 0; i < linhas; i++) {
        mapa->matriz[i] = memAlocar((size_t)colunas * sizeof(char));
        if (mapa->matriz[i] == NULL) {
            mapa->linhas = i; // so libera as linhas que chegaram a ser alocadas
            mapaLiberar(mapa);
            fclose(f);
            return 0;
        }

        if (fgets(buffer, sizeof(buffer), f) == NULL) buffer[0] = '\0';

        // Se a linha do arquivo for mais curta que o mapa (ou acabar em \r\n,
        // como no Windows), o que falta vira chao vazio
        int fimLinha = 0;
        for (int j = 0; j < colunas; j++) {
            char elementoMatriz = fimLinha ? ' ' : buffer[j];

            if (elementoMatriz == '\n' || elementoMatriz == '\r' || elementoMatriz == '\0') {
                fimLinha = 1;
                elementoMatriz = ' ';
            }

            // Piso que fica embaixo de um objeto: copia o da casa a esquerda
            // (corredor '.' ou sala ' '). Se a esquerda for parede, vira corredor.
            char piso = '.';
            if (j > 0 && (mapa->matriz[i][j - 1] == '.' || mapa->matriz[i][j - 1] == ' ')) {
                piso = mapa->matriz[i][j - 1];
            }

            // Leitura das letras do mapa
            switch (elementoMatriz) {
                case 'P':
                    // Ponto de partida do jogador neste andar
                    jogo->jogador.posicaoInicial.linha = i;
                    jogo->jogador.posicaoInicial.coluna = j;
                    jogo->jogador.posicao = jogo->jogador.posicaoInicial;
                    mapa->matriz[i][j] = piso;
                    break;
                case 'S':
                    // Escada: guardamos a posicao para o main saber quando o jogador sobe
                    mapa->saida.linha = i;
                    mapa->saida.coluna = j;
                    mapa->matriz[i][j] = piso;
                    break;
                case 'I':
                    // Bateria: entra na lista encadeada de itens do jogo
                    adicionar_item(&jogo->itens, (Posicao){ .linha = i, .coluna = j }, ITEM_BATERIA);
                    mapa->matriz[i][j] = piso;
                    break;
                case 'F':
                    // Professor: a posicao vem do arquivo; a pergunta e definida por andar
                    jogo->professor.posicao = (Posicao){ .linha = i, .coluna = j };
                    mapa->matriz[i][j] = piso;
                    break;
               case 'R': // Robo Roxo
                    robosAdicionar(&jogo->robos, (Posicao){ .linha = i, .coluna = j }, ROBO_ROXO, jogo->andarAtual);
                    mapa->matriz[i][j] = piso;
                    break;
                case 'X': // Robo Vermelho
                    robosAdicionar(&jogo->robos, (Posicao){ .linha = i, .coluna = j }, ROBO_VERMELHO, jogo->andarAtual);
                    mapa->matriz[i][j] = piso;
                    break;
                case 'Z': // Robo Laranja
                    robosAdicionar(&jogo->robos, (Posicao){ .linha = i, .coluna = j }, ROBO_LARANJA, jogo->andarAtual);
                    mapa->matriz[i][j] = piso;
                    break;
                case 'B': // Robo Cinza
                    robosAdicionar(&jogo->robos, (Posicao){ .linha = i, .coluna = j }, ROBO_CINZA, jogo->andarAtual);
                    mapa->matriz[i][j] = piso;
                    break;
                case '#': // parede
                case '.': // piso de corredor
                case ' ': // piso de sala
                case '=': // tapete com a logo (andavel)
                case 'M': // mesa
                case 'C': // computador
                    mapa->matriz[i][j] = elementoMatriz;
                    break;
                default:
                    mapa->matriz[i][j] = piso;
                    break;
            }
        }
    }

    fclose(f);

    // Sem professor a escada nunca seria liberada: trata como arquivo invalido
    if (jogo->professor.posicao.linha < 0) {
        printf("Erro: %s nao tem professor (letra F)\n", arquivo);
        liberar_todos_itens(&jogo->itens);
        mapaLiberar(mapa);
        return 0;
    }
    return 1;
}

// Troca de andar
int mapaCarregarAndar(Jogo *jogo, int andar) {
    if (jogo == NULL || andar < 1 || andar > TOTAL_ANDARES) return 0;

    //libera tudo antes de trocar pelos novos elementos
    liberar_todos_itens(&jogo->itens);
    robosLiberar(&jogo->robos); // senao os robos do andar anterior continuariam neste
    mapaLiberar(&jogo->mapa);

    // Monta o nome do arquivo a partir do numero do andar.
    char arquivo[64];
    snprintf(arquivo, sizeof(arquivo), "andares/andar%d.txt", andar);
    if (!mapaCarregar(jogo, arquivo)) return 0;

    inicializar_professor_andar(&jogo->professor, jogo->professor.posicao, andar);
    jogo->jogador.temPendrive = false; //pra fechar a proxima escada

    // Guarda quantas baterias o andar tem, para o HUD mostrar "coletadas/total"
    jogo->totalItensAndar = contar_itens(jogo->itens);

    // Vidas, pontos e bateria do jogador nao sao tocados: passam de um andar para o outro.
    // So a posicao muda, porque o mapaCarregar coloca o jogador no 'P' do novo andar.
    jogo->andarAtual = andar;
    return 1;
}

void mapaLiberar(Mapa *mapa) {
    if (mapa == NULL || mapa->matriz == NULL) return;

    for (int i = 0; i < mapa->linhas; i++) {
        memLiberar(mapa->matriz[i]);
    }
    memLiberar(mapa->matriz);

    mapa->matriz = NULL;
    mapa->linhas = 0;
    mapa->colunas = 0;
}


// Percorre a matriz e desenha cada casa conforme a letra dela.
// origem e tamanhoCasa permitem desenhar o mapa grande (2D) e o minimapa (3D)
// com a mesma funcao.
void desenharMapa2D(Mapa *mapa, Vector2 origem, int tamanhoCasa) {
    if(mapa == NULL || mapa->matriz == NULL) return;

    for(int i = 0; i < mapa->linhas; i++){
        for(int j = 0; j < mapa->colunas; j++){
            // Converte linha/coluna em pixels a partir da origem
            int x = (int)origem.x + j * tamanhoCasa;
            int y = (int)origem.y + i * tamanhoCasa;

            switch (mapa->matriz[i][j]) {
                case '#':
                    desenharParede2D(mapa, i, j, x, y, tamanhoCasa);
                    break;
                case 'M':
                    // Moveis ficam em salas: primeiro o piso de sala, depois o movel por cima
                    desenharPiso2D(mapa, i, j, x, y, tamanhoCasa, ' ');
                    desenharMesa2D(x, y, tamanhoCasa);
                    break;
                case 'C':
                    desenharPiso2D(mapa, i, j, x, y, tamanhoCasa, ' ');
                    desenharComputador2D(x, y, tamanhoCasa);
                    break;
                default:
                    // '.' (corredor) ou ' ' (sala)
                    desenharPiso2D(mapa, i, j, x, y, tamanhoCasa, mapa->matriz[i][j]);
                    if (mapaTemTapete(mapa, i, j)) DrawRectangle(x, y, tamanhoCasa, tamanhoCasa, COR_CESAR_LARANJA);
                    break;
            }
        }
    }

    // Logos dos tapetes: depois de tudo, senao as casas seguintes do proprio
    // tapete cobririam o desenho. No minimapa fica so o retangulo laranja.
    if (tamanhoCasa < DETALHE_MINIMO) return;
    for (int i = 0; i < mapa->linhas; i++) {
        for (int j = 0; j < mapa->colunas; j++) {
            int largura, altura;
            if (!mapaCantoTapete(mapa, i, j, &largura, &altura)) continue;
            desenharLogoTapete2D((int)origem.x + j * tamanhoCasa, (int)origem.y + i * tamanhoCasa,
                                 largura * tamanhoCasa, altura * tamanhoCasa);
        }
    }
}

// Desenha a escada (saida do andar) na casa do 'S', com degraus
void desenharSaida2D(Mapa *mapa, Vector2 origem, int tamanhoCasa, bool liberada) {
    if(mapa == NULL || mapa->saida.linha < 0) return; // andar sem escada

    // Escada fica cinza enquanto o professor nao foi ajudado e verde quando o jogador pode subir
    Color claro  = liberada ? (Color){ 0, 210, 90, 255 } : (Color){ 135, 135, 140, 255 };
    Color escuro = liberada ? (Color){ 0, 105, 45, 255 } : (Color){  65,  65,  70, 255 };

    int x = (int)origem.x + mapa->saida.coluna * tamanhoCasa;
    int y = (int)origem.y + mapa->saida.linha * tamanhoCasa;

    // No minimapa a casa e pequena demais para degraus: so um quadrado
    if (tamanhoCasa < DETALHE_MINIMO) {
        DrawRectangle(x, y, tamanhoCasa, tamanhoCasa, claro);
        return;
    }

    // Borda de 2px para a escada nao encostar nas paredes vizinhas
    int m = 2;
    int largura = tamanhoCasa - 2 * m;
    DrawRectangle(x + m, y + m, largura, largura, escuro);

    // 4 degraus: o de cima (mais alto) e o mais claro, os de baixo vao
    // escurecendo. O -1 na altura deixa uma linha escura entre os degraus.
    int degraus = 4;
    int altura = largura / degraus;
    for (int k = 0; k < degraus; k++) {
        Color cor = misturarCor(claro, escuro, (float)k / (float)(degraus - 1));
        DrawRectangle(x + m, y + m + k * altura, largura, altura - 1, cor);
    }
}


//funcoes internas

// Mistura duas cores: fator 0 = so a cor a, fator 1 = so a cor b
static Color misturarCor(Color a, Color b, float fator) {
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * fator),
        (unsigned char)(a.g + (b.g - a.g) * fator),
        (unsigned char)(a.b + (b.b - a.b) * fator),
        255
    };
}

// Letra da casa, ou '#' fora do mapa: assim a beirada conta como parede e
// ninguem precisa conferir os limites de novo
static char letraCasa(Mapa *mapa, int linha, int coluna) {
    if (linha < 0 || linha >= mapa->linhas || coluna < 0 || coluna >= mapa->colunas) return '#';
    return mapa->matriz[linha][coluna];
}

// true se a casa e parede (ou fica fora do mapa)
static bool ehParede(Mapa *mapa, int linha, int coluna) {
    return letraCasa(mapa, linha, coluna) == '#';
}

// Diz se o jogador pode entrar numa casa. Fica no mapa.c porque e o mapa
// que sabe o que e cada letra; o movimento (jogador.c) so pergunta aqui.
bool mapaCasaBloqueada(Mapa *mapa, int linha, int coluna) {
    if (mapa == NULL || mapa->matriz == NULL) return true;

    // Parede (ou fora do mapa) e moveis (mesa e computador) bloqueiam; o resto e piso
    char c = letraCasa(mapa, linha, coluna);
    return c == '#' || c == 'M' || c == 'C';
}

// Parede com volume: a casa e pintada como a "tampa" da parede e cada lado
// que encosta em piso ganha uma borda. Luz vem de cima/esquerda, sombra fica
// na direita, e embaixo aparece a face da frente (como se a camera estivesse
// um pouco inclinada).
static void desenharParede2D(Mapa *mapa, int i, int j, int x, int y, int t) {
    int borda  = (t / 12 > 0) ? t / 12 : 1;
    int frente = (t / 4 > 0) ? t / 4 : 1;

    DrawRectangle(x, y, t, t, COR_PAREDE_TOPO);

    if (t >= DETALHE_MINIMO) {
        // Efeitos: emenda escura entre os paineis da parede e, em algumas
        // paredes, uma forma do guia de marca (circulo ou meio circulo)
        DrawRectangleLines(x, y, t, t, Fade(BLACK, 0.12f));
        DrawRectangleLines(x + borda + 1, y + borda + 1, t - 2 * borda - 2, t - 2 * borda - 2, Fade(WHITE, 0.10f));
        if ((i * 5 + j * 3) % 7 == 0) {
            float r = (float)t * 0.2f;
            if ((i + j) % 2 == 0) DrawCircle(x + t / 2, y + t / 2, r, COR_CESAR_LARANJA2);
            else DrawCircleSector((Vector2){ (float)x + (float)t / 2.0f, (float)y + (float)t / 2.0f + r / 2.0f }, r, 180.0f, 360.0f, 16, COR_CESAR_LARANJA2);
        }
    }

    if (!ehParede(mapa, i - 1, j)) DrawRectangle(x, y, t, borda, COR_PAREDE_LUZ);
    if (!ehParede(mapa, i, j - 1)) DrawRectangle(x, y, borda, t, COR_PAREDE_LUZ);
    if (!ehParede(mapa, i, j + 1)) DrawRectangle(x + t - borda, y, borda, t, COR_PAREDE_SOMBRA);
    if (!ehParede(mapa, i + 1, j)) {
        // Faixa laranja da frente: e a luz de emergencia do predio, que pulsa devagar
        float pulso = 0.8f + 0.2f * sinf((float)GetTime() * 2.0f);
        DrawRectangle(x, y + t - frente, t, frente, misturarCor(COR_CESAR_LARANJA2, COR_CESAR_LARANJA, pulso));
        DrawRectangle(x, y + t - frente, t, 1, COR_PAREDE_SOMBRA);
    }
}

// Piso: corredor ('.') e uma placa cinza com rejunte; sala (' ') e um xadrez
// de duas cores quentes. Logo abaixo de uma parede o piso recebe a sombra dela.
static void desenharPiso2D(Mapa *mapa, int i, int j, int x, int y, int t, char tipo) {
    if (tipo == '.') {
        DrawRectangle(x, y, t, t, COR_CORREDOR);
        if (t >= DETALHE_MINIMO) DrawRectangleLines(x, y, t, t, COR_CORREDOR_LINHA);
    } else {
        // (i + j) par/impar alterna as cores, formando o xadrez
        DrawRectangle(x, y, t, t, ((i + j) % 2 == 0) ? COR_SALA_A : COR_SALA_B);
    }

    // Sombra: degrade de preto (em cima) ate transparente
    if (t >= DETALHE_MINIMO && ehParede(mapa, i - 1, j)) {
        DrawRectangleGradientV(x, y, t, t / 3, Fade(BLACK, 0.45f), Fade(BLACK, 0.0f));
    }
}

// Tampo de movel visto de cima, com a frente mais escura para dar volume
static void desenharTampo2D(int x, int y, int t, Color tampo, Color frente) {
    int m = (t / 8 > 0) ? t / 8 : 0; // margem para o movel nao encostar nas casas vizinhas
    DrawRectangle(x + m, y + m, t - 2 * m, t - 2 * m, tampo);
    if (t >= DETALHE_MINIMO) {
        DrawRectangle(x + m, y + t - m - t / 6, t - 2 * m, t / 6, frente);
        DrawRectangleLines(x + m, y + m, t - 2 * m, t - 2 * m, frente);
    }
}

// Mesa: tampo de madeira (o desenho comum fica no desenharTampo2D)
static void desenharMesa2D(int x, int y, int t) {
    desenharTampo2D(x, y, t, COR_MADEIRA, COR_MADEIRA_ESCURA);
}

// Computador: mesa de metal com monitor (tela azul acesa) e teclado
static void desenharComputador2D(int x, int y, int t) {
    desenharTampo2D(x, y, t, COR_METAL, COR_METAL_ESCURO);
    if (t < DETALHE_MINIMO) return;

    DrawRectangle(x + t / 4, y + t / 6, t / 2, t / 3, (Color){ 20, 20, 28, 255 }); // carcaca do monitor
    DrawRectangle(x + t / 4 + 2, y + t / 6 + 2, t / 2 - 4, t / 3 - 4, COR_TELA);    // tela
    DrawRectangle(x + t / 4, y + t * 6 / 10, t / 2, t / 8, LIGHTGRAY);              // teclado
}

// Tapete: no arquivo do andar, um retangulo de '=' vira um tapete so, com a
// logo do CESAR no meio. Devolve true se a casa e tapete.
bool mapaTemTapete(Mapa *mapa, int linha, int coluna) {
    if (mapa == NULL || mapa->matriz == NULL) return false;
    return letraCasa(mapa, linha, coluna) == '=';
}

// true se a casa e o canto de cima/esquerda de um tapete; nesse caso devolve
// o tamanho dele em casas. A logo e desenhada uma vez so, a partir do canto.
bool mapaCantoTapete(Mapa *mapa, int linha, int coluna, int *largura, int *altura) {
    if (!mapaTemTapete(mapa, linha, coluna)) return false;
    if (letraCasa(mapa, linha - 1, coluna) == '=' || letraCasa(mapa, linha, coluna - 1) == '=') return false;

    int l = 0, a = 0;
    while (letraCasa(mapa, linha, coluna + l) == '=') l++;
    while (letraCasa(mapa, linha + a, coluna) == '=') a++;
    if (largura != NULL) *largura = l;
    if (altura != NULL) *altura = a;
    return true;
}

// Logo do CESAR em claro sobre o tapete laranja (versao do logo sobre fundo
// laranja do guia de marca), centralizada no retangulo (x, y, largura, altura)
// em pixels. O simbolo e feito de elipses: a grande e, dentro dela, dois
// aneis laranja abertos em cima. Com espaco, vem o nome embaixo.
static void desenharLogoTapete2D(int x, int y, int largura, int altura) {
    DrawRectangleLinesEx((Rectangle){ (float)x + 2, (float)y + 2, (float)largura - 4, (float)altura - 4 }, 2.0f, COR_CESAR_LARANJA2);

    // O simbolo e 2x mais largo que alto: cabe em 70% da largura ou 45% da altura
    float rx = fminf((float)largura * 0.35f, (float)altura * 0.45f), ry = rx / 2.0f;
    float cx = (float)x + (float)largura / 2.0f, cy = (float)y + (float)altura / 2.0f;

    int fonte = (int)(ry * 0.7f);
    bool comNome = fonte >= 10;          // abaixo disso o texto fica ilegivel
    if (comNome) cy -= ry * 0.45f;       // sobe o simbolo para o nome caber embaixo

    float ay = cy - ry * 0.28f;          // os aneis ficam um pouco acima do centro
    DrawEllipse((int)cx, (int)cy, rx, ry, COR_CESAR_CLARO);
    DrawEllipse((int)cx, (int)ay, rx * 0.62f, ry * 0.58f, COR_CESAR_LARANJA);  // anel de fora
    DrawEllipse((int)cx, (int)ay, rx * 0.44f, ry * 0.38f, COR_CESAR_CLARO);
    DrawEllipse((int)cx, (int)ay, rx * 0.26f, ry * 0.20f, COR_CESAR_LARANJA);  // anel de dentro
    DrawRectangle((int)(cx - rx * 0.08f), (int)(ay - ry * 0.62f), (int)(rx * 0.16f) + 1, (int)(ry * 0.45f) + 1, COR_CESAR_CLARO);

    if (comNome) {
        const char *nome = "C E S A R";
        DrawText(nome, (int)cx - MeasureText(nome, fonte) / 2, (int)(cy + ry * 1.25f), fonte, COR_CESAR_CLARO);
    }
}

