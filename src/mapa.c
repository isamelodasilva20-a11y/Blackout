#include <stdio.h>
#include <stddef.h>
#include "raylib.h"

// 1. OBRIGATÓRIO: jogo.h vem PRIMEIRO porque define as structs base
#include "jogo.h"

// 2. Depois os restantes ficheiros de cabeçalho do projeto
#include "mapa.h"
#include "memoria.h"
#include "itens.h"
#include "professor.h"

#define TAM_BUFFER 256

// Abaixo desse tamanho de casa (o minimapa do modo 3D) os detalhes nao
// aparecem direito, entao desenhamos so as cores basicas
#define DETALHE_MINIMO 12

// Cores do mapa 2D
#define COR_PAREDE_TOPO    (Color){  30,  62, 120, 255 } // "tampa" da parede vista de cima
#define COR_PAREDE_LUZ     (Color){  70, 110, 180, 255 } // borda de cima/esquerda (luz)
#define COR_PAREDE_SOMBRA  (Color){  18,  38,  75, 255 } // borda da direita (sombra)
#define COR_PAREDE_FRENTE  (Color){  14,  28,  56, 255 } // face da frente, vista "de lado"
#define COR_CORREDOR       (Color){  44,  46,  54, 255 }
#define COR_CORREDOR_LINHA (Color){  36,  38,  45, 255 } // rejunte entre as placas
#define COR_SALA_A         (Color){  62,  52,  42, 255 } // piso de sala em xadrez (duas cores)
#define COR_SALA_B         (Color){  56,  47,  38, 255 }
#define COR_MADEIRA        (Color){ 125,  85,  50, 255 }
#define COR_MADEIRA_ESCURA (Color){  80,  52,  30, 255 }
#define COR_METAL          (Color){  95,  98, 108, 255 }
#define COR_METAL_ESCURO   (Color){  60,  62,  70, 255 }
#define COR_TELA           (Color){  80, 170, 235, 255 }

// Mistura duas cores: fator 0 = so a cor a, fator 1 = so a cor b
static Color misturarCor(Color a, Color b, float fator) {
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * fator),
        (unsigned char)(a.g + (b.g - a.g) * fator),
        (unsigned char)(a.b + (b.b - a.b) * fator),
        255
    };
}

// true se a casa e parede. Fora do mapa conta como parede, para nao
// desenhar bordas na beirada da tela.
static bool ehParede(Mapa *mapa, int linha, int coluna) {
    if (linha < 0 || linha >= mapa->linhas || coluna < 0 || coluna >= mapa->colunas) return true;
    return mapa->matriz[linha][coluna] == '#';
}

// Diz se o jogador pode entrar numa casa. Fica no mapa.c porque e o mapa
// que sabe o que e cada letra; o movimento (jogador.c) so pergunta aqui.
bool mapaCasaBloqueada(Mapa *mapa, int linha, int coluna) {
    if (mapa == NULL || mapa->matriz == NULL) return true;
    // Fora dos limites do mapa nao da para andar
    if (linha < 0 || linha >= mapa->linhas || coluna < 0 || coluna >= mapa->colunas) return true;

    // Parede e moveis (mesa e computador) bloqueiam; o resto e piso
    char c = mapa->matriz[linha][coluna];
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
    if (!ehParede(mapa, i - 1, j)) DrawRectangle(x, y, t, borda, COR_PAREDE_LUZ);
    if (!ehParede(mapa, i, j - 1)) DrawRectangle(x, y, borda, t, COR_PAREDE_LUZ);
    if (!ehParede(mapa, i, j + 1)) DrawRectangle(x + t - borda, y, borda, t, COR_PAREDE_SOMBRA);
    if (!ehParede(mapa, i + 1, j)) DrawRectangle(x, y + t - frente, t, frente, COR_PAREDE_FRENTE);
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
                    break;
            }
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


// A legenda dos caracteres do mapa esta em mapa.h
int mapaCarregar(Jogo *jogo, const char *arquivo) {
    if (jogo == NULL || arquivo == NULL) return 0;

    FILE *f = fopen(arquivo, "r");
    if (f == NULL) {
        printf("Erro: nao achei o arquivo %s\n", arquivo);
        return 0;
    }

    // 1) Cabecalho: tamanho do mapa ("25 40")
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

    // 3) Le o mapa linha por linha, casa por casa
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
            char c = fimLinha ? ' ' : buffer[j];

            if (c == '\n' || c == '\r' || c == '\0') {
                fimLinha = 1;
                c = ' ';
            }

            // Piso que fica embaixo de um objeto: copia o da casa a esquerda
            // (corredor '.' ou sala ' '). Se a esquerda for parede, vira corredor.
            char piso = '.';
            if (j > 0 && (mapa->matriz[i][j - 1] == '.' || mapa->matriz[i][j - 1] == ' ')) {
                piso = mapa->matriz[i][j - 1];
            }

            // Letras especiais sao lidas aqui e viram piso na matriz:
            // a matriz guarda so o cenario, e os objetos ficam nas suas proprias structs
            switch (c) {
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
                    // Professor: a posicao vem do arquivo; a pergunta e definida
                    // depois, no mapaCarregarAndar, de acordo com o andar
                    jogo->professor.posicao = (Posicao){ .linha = i, .coluna = j };
                    mapa->matriz[i][j] = piso;
                    break;
                case 'R': // robos: reservados para a parte dos robos,
                case 'X': // que vai ler essas letras aqui. Por enquanto
                case 'Z': // viram piso.
                case 'L': // lanterna agora e fixa, nao vira item
                    mapa->matriz[i][j] = piso;
                    break;
                case '#': // parede
                case '.': // piso de corredor
                case ' ': // piso de sala
                case 'M': // mesa
                case 'C': // computador
                    mapa->matriz[i][j] = c;
                    break;
                default:
                    // Caractere desconhecido: vira piso para nao travar o jogo
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

// Troca de andar: descarta o mapa e os itens atuais e carrega andares/andarN.txt
// Usada tanto no inicio do jogo (andar 1) quanto quando o jogador sobe a escada.
int mapaCarregarAndar(Jogo *jogo, int andar) {
    if (jogo == NULL || andar < 1 || andar > TOTAL_ANDARES) return 0;

    // Limpa o andar anterior antes de carregar o novo, para nao vazar memoria.
    // No primeiro andar a lista e a matriz ainda estao vazias (NULL), e as
    // duas funcoes simplesmente nao fazem nada.
    liberar_todos_itens(&jogo->itens);
    mapaLiberar(&jogo->mapa);

    // Monta o nome do arquivo a partir do numero do andar: 2 -> "andares/andar2.txt"
    char arquivo[64];
    snprintf(arquivo, sizeof(arquivo), "andares/andar%d.txt", andar);
    if (!mapaCarregar(jogo, arquivo)) return 0;

    // Cada andar tem o seu professor, que comeca sem ter sido ajudado
    // (ajudado = false) e com a pergunta deste andar. O jogador tambem
    // perde o pendrive do andar anterior. Sem isso a escada dos andares
    // 2 e 3 ja comecaria liberada.
    inicializar_professor_andar(&jogo->professor, jogo->professor.posicao, andar);
    jogo->jogador.temPendrive = false;

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
