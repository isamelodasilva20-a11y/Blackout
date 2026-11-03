#include <stddef.h>
#include <string.h>

#include "professor.h"
#include "hud.h"
#include "raylib.h"

//atribui as carasterísticas do professor
void inicializar_professor(Professor *professor, Posicao posicao, const char *nome, const char *pergunta, const char *resposta){
    //verifica se o professor existe
    if(professor==NULL)return;
    professor->posicao = posicao;
    professor->nome = nome;
    professor->pergunta = pergunta;
    professor->resposta = resposta;
    professor->ajudado = false;
}

//uma pergunta diferente para cada andar (posição 0 = andar 1)
typedef struct {
    const char *pergunta;
    const char *resposta;
} PerguntaAndar;

static const PerguntaAndar PERGUNTAS_ANDAR[] = {
    { "Qual especificador imprime um int em C?\nA) %s  B) %d  C) %f", "B" },
    { "Qual funcao libera a memoria\nalocada com malloc?\nA) free  B) delete  C) clear  D) drop", "A" },
    { "Quantos bytes ocupa um char em C?\nA) 4  B) 2  C) 1  D) 8", "C" },
};

//escolhe a pergunta do andar e reaproveita o inicializar_professor,
//que também volta o ajudado para false
void inicializar_professor_andar(Professor *professor, Posicao posicao, int andar){
    int total = sizeof(PERGUNTAS_ANDAR) / sizeof(PERGUNTAS_ANDAR[0]);
    //se existirem mais andares que perguntas, elas se repetem
    int indice = (andar - 1) % total;
    if(indice < 0) indice = 0;
    inicializar_professor(professor, posicao, "Prof. Thiaguinho", PERGUNTAS_ANDAR[indice].pergunta, PERGUNTAS_ANDAR[indice].resposta);
}

//verifica se o jogador está falando com o professor
//biblioteca bool declarada no jogo.h
bool verificar_interacao_professor(Jogador *jogador, Professor *professor){
    if(jogador==NULL || professor ==NULL || professor->ajudado)return false;
    if(jogador->posicao.linha == professor->posicao.linha && jogador->posicao.coluna == professor->posicao.coluna)return true;
    return false;
}

//desenha a representação visual dos professor
//origem = canto superior esquerdo do mapa na tela (no minimapa ela muda)
void desenhar_professor(Professor *professor, Vector2 origem, int tamanhoCasa){
    if(professor==NULL || professor->posicao.linha < 0)return;

    //pega a posição exata do professor
    int centroX = (int)origem.x + (professor->posicao.coluna * tamanhoCasa) + (tamanhoCasa / 2);
    int centroY = (int)origem.y + (professor->posicao.linha * tamanhoCasa) + (tamanhoCasa / 2);

    //diferencia a cor do professor de acordo com seu status ajudado
    Color corProfessor = professor->ajudado ? GRAY : PURPLE;
    DrawCircle(centroX, centroY, (float)tamanhoCasa / 3.0f, corProfessor);

    //se não tiver sido ajudado possuirá uma ! na cabeça (no minimapa não cabe)
    if (!professor->ajudado && tamanhoCasa >= 12) {
        DrawText("!", centroX - 2, centroY - (tamanhoCasa / 2) - 8, 10, YELLOW);
        //centroX pega o centro horizontal
        //centroY pega o centro vertical porem flutuando 8px acima
        //tamanho 10 de letra
    }
}

//converte uma medida do tamanho base da janela (1024x640) para a janela atual
static int escalar(int valorBase, float escala){
    return (int)((float)valorBase * escala);
}

//tamanho de fonte escalado, nunca menor que 10 (mínimo legível)
static int fonteQuiz(int tamanhoBase, float escala){
    int tamanho = escalar(tamanhoBase, escala);
    return tamanho < 10 ? 10 : tamanho;
}

//janela do quiz: escurece o jogo, mostra a pergunta e lê a resposta (A, B, C ou D).
//depois mostra por 1,5 s se acertou ou errou; só fecha quando o jogador acerta
void desenhar_quiz(Jogo *jogo) {
    if (jogo == NULL) return;

    //todas as medidas do quiz crescem/diminuem junto com a janela
    float e = hudEscala();

    //efeito de pause
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f));

    //tela principal
    int larguraModal = escalar(400, e);
    int alturaModal = escalar(220, e);
    int posX = (GetScreenWidth() - larguraModal) / 2;
    int posY = (GetScreenHeight() - alturaModal) / 2;

    DrawRectangle(posX, posY, larguraModal, alturaModal, DARKGRAY);
    DrawRectangleLines(posX, posY, larguraModal, alturaModal, RAYWHITE);

    // nome do professor e pergunta
    DrawText(jogo->professor.nome, posX + escalar(20, e), posY + escalar(15, e), fonteQuiz(20, e), YELLOW);
    DrawText(jogo->professor.pergunta, posX + escalar(20, e), posY + escalar(50, e), fonteQuiz(16, e), WHITE);

    if (jogo->estadoFeedback == FEEDBACK_NENHUM) {
        // Mostra a instrução enquanto aguarda resposta
        DrawText("Pressione [A], [B], [C] ou [D] para responder", posX + escalar(20, e), posY + escalar(190, e), fonteQuiz(12, e), LIGHTGRAY);

        //verifica a resposta do teclado
        const char *respostaJogador = NULL;
        if (IsKeyPressed(KEY_A)) respostaJogador = "A";
        if (IsKeyPressed(KEY_B)) respostaJogador = "B";
        if (IsKeyPressed(KEY_C)) respostaJogador = "C";
        if (IsKeyPressed(KEY_D)) respostaJogador = "D";

        if (respostaJogador != NULL) {
            jogo->tempoInicioFeedback = GetTime();

            //compara a resposta com o gabarito
            if (strcmp(respostaJogador, jogo->professor.resposta) == 0) {
                jogo->estadoFeedback = FEEDBACK_ACERTOU;
                concluir_ajuda_professor(&jogo->jogador, &jogo->professor);
            } else {
                jogo->estadoFeedback = FEEDBACK_ERROU;
            }
        }
    } 
    else {
        //Renderização visual do Feedback (Acertou / Errou)
        if (jogo->estadoFeedback == FEEDBACK_ACERTOU) {
            DrawRectangle(posX + escalar(20, e), posY + escalar(180, e), larguraModal - escalar(40, e), escalar(40, e), GREEN);
            DrawText("RESPOSTA CORRETA! Pendrive obtido!", posX + escalar(35, e), posY + escalar(192, e), fonteQuiz(16, e), WHITE);
        } 
        else if (jogo->estadoFeedback == FEEDBACK_ERROU) {
            DrawRectangle(posX + escalar(20, e), posY + escalar(180, e), larguraModal - escalar(40, e), escalar(40, e), RED);
            DrawText("INCORRETO! Tente novamente.", posX + escalar(60, e), posY + escalar(192, e), fonteQuiz(16, e), WHITE);
        }

        //conta um tempo pra voltar pra tela inicial
        if (GetTime() - jogo->tempoInicioFeedback >= 1.5) {
            if (jogo->estadoFeedback == FEEDBACK_ACERTOU) {
                jogo->telaAtual = TELA_NORMAL;
            }
            jogo->estadoFeedback = FEEDBACK_NENHUM;
        }
    }
}

//entrega o pendrive ao jogador, recarrega a bateria e muda a cor do professor
void concluir_ajuda_professor(Jogador *jogador, Professor *professor){
    if(jogador == NULL || professor == NULL || professor->ajudado)return;

    professor->ajudado = true;
    jogador->temPendrive = true;
    jogador->bateria = 100;
}