#include <stddef.h>
#include <string.h>

#include "professor.h"
#include "hud.h"
#include "raylib.h"

// Escala para o sprite ficar com a mesma altura visual do jogador 
#define ESCALA_SPRITE 1.4

typedef struct {
    const char *nome;
    const char *pergunta;
    const char *resposta;
    TipoProfessor tipo;
} DadosProfessorAndar;

static const DadosProfessorAndar DADOS_PROFESSORES[] = {
    { "Prof. Thiaguinho", "Qual especificador imprime \num int em C?\nA) %s  B) %d  C) %f", "B", PROFESSOR_1 },
    { "Prof. Daniel",      "Qual função libera a memória\nalocada com malloc?\nA) free  \nB) delete  \nC) clear  \nD) drop", "A", PROFESSOR_2 },
    { "Prof. Péricles",     "Qual a principal funcao da \nplataforma Eden AI?\nA) Criar imagens 3D\nB) Agrupar conteudos e tutoria de estudos\nC) Compilar codigo C\nD) Formatar o computador", "B", PROFESSOR_3 },
};

// atribui as características do professor
void inicializar_professor(Professor *professor, Posicao posicao, const char *nome, const char *pergunta, const char *resposta, TipoProfessor tipo) {
    if (professor == NULL) return;
    professor->posicao = posicao;
    professor->nome = nome;
    professor->pergunta = pergunta;
    professor->resposta = resposta;
    professor->id = tipo;
    professor->ajudado = false;
}

// escolhe a pergunta do andar e reaproveita o inicializar_professor,
// que também volta o ajudado para false
void inicializar_professor_andar(Professor *professor, Posicao posicao, int andar) {
    int total = sizeof(DADOS_PROFESSORES) / sizeof(DADOS_PROFESSORES[0]);
    // se existirem mais andares que perguntas, elas se repetem
    int indice = (andar - 1) % total;
    if (indice < 0) indice = 0;

    inicializar_professor(
        professor,
        posicao,
        DADOS_PROFESSORES[indice].nome,
        DADOS_PROFESSORES[indice].pergunta,
        DADOS_PROFESSORES[indice].resposta,
        DADOS_PROFESSORES[indice].tipo
    );
}

// verifica se o jogador está falando com o professor
// biblioteca bool declarada no jogo.h
bool verificar_interacao_professor(Jogador *jogador, Professor *professor) {
    if (jogador == NULL || professor == NULL || professor->ajudado) return false;
    if (jogador->posicao.linha == professor->posicao.linha && jogador->posicao.coluna == professor->posicao.coluna) return true;
    return false;
}

void desenhar_professor(const Jogo *jogo, Vector2 origem, int tamanhoCasa) {
    if (jogo == NULL || jogo->professor.posicao.linha < 0) return;

    const Professor *professor = &jogo->professor;

    float tamanho = (float)tamanhoCasa * ESCALA_SPRITE;

    // Posição horizontal centralizada na casa
    float posX = origem.x + (float)(professor->posicao.coluna * tamanhoCasa) + ((float)tamanhoCasa / 2.0f);
    
    // Ancoragem perfeita: os pés assentam exatamente na borda inferior da casa
    float posY = origem.y + (float)(professor->posicao.linha * tamanhoCasa + tamanhoCasa) - tamanho;

    // Seleciona a textura do professor
    int profID = professor->id;
    int poseIndex = professor->ajudado ? 1 : 0;
    Texture2D tex = jogo->texturasProfessores[profID][poseIndex];

    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
    Rectangle dest = { posX, posY, tamanho, tamanho };
    Vector2 origin = { tamanho / 2.0f, 0.0f };

    // Sombra oval no chão (alinhada aos pés na base da casa)
    DrawEllipse((int)posX, (int)(origem.y + (float)(professor->posicao.linha * tamanhoCasa + tamanhoCasa) - 3.0f),
                (float)tamanhoCasa * 0.45f, (float)tamanhoCasa * 0.15f, Fade(BLACK, 0.4f));

    // Desenha o sprite
    DrawTexturePro(tex, src, dest, origin, 0.0f, WHITE);

    // Ícone '!' ajustado acima da cabeça
    if (!professor->ajudado && tamanhoCasa >= 12) {
        DrawText("!", (int)posX - 3, (int)posY - 12, 14, YELLOW);
    }
}

// converte uma medida do tamanho base da janela (1024x640) para a janela atual
static int escalar(int valorBase, float escala) {
    return (int)((float)valorBase * escala);
}

// tamanho de fonte escalado, nunca menor que 10 (mínimo legível)
static int fonteQuiz(int tamanhoBase, float escala) {
    int tamanho = escalar(tamanhoBase, escala);
    return tamanho < 10 ? 10 : tamanho;
}

// janela do quiz: escurece o jogo, mostra a pergunta e lê a resposta (A, B, C ou D).
// depois mostra por 1,5 s se acertou ou errou; só fecha quando o jogador acerta
void desenhar_quiz(Jogo *jogo) {
    if (jogo == NULL) return;

    // todas as medidas do quiz crescem/diminuem junto com a janela
    float e = hudEscala();

    // efeito de pause
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f));

    // tela principal: professor à esquerda, aluno à direita e a caixa de fala no meio
    int L = GetScreenWidth();
    int A = GetScreenHeight();
    float altPers = A * 0.45f;
    Rectangle destProf  = { L * 0.03f, A - altPers - A * 0.05f, altPers, altPers };
    Rectangle destAluno = { L - L * 0.03f - altPers, destProf.y, altPers, altPers };
    float xCaixa = destProf.x + destProf.width + L * 0.02f;
    Rectangle caixa = { xCaixa, A * 0.40f, destAluno.x - L * 0.02f - xCaixa, A * 0.50f };

    // efeito de digitação: a pergunta aparece 40 letras por segundo; [Espaço] pula pro final
    const char *pergunta = jogo->professor.pergunta;
    int letras = (int)(jogo->tempoFala * 40);
    bool terminou = letras >= (int)TextLength(pergunta);
    if (!terminou && IsKeyPressed(KEY_SPACE)) {
        jogo->tempoFala = 1000.0f;
        terminou = true;
    }

    // retrato do professor (pose 1 quando o jogador acerta)
    int pose = (jogo->estadoFeedback == FEEDBACK_ACERTOU) ? 1 : 0;
    Texture2D texProf = jogo->texturasProfessores[jogo->professor.id][pose];
    Rectangle recProf = { 0, 0, (float)texProf.width, (float)texProf.height };
    DrawTexturePro(texProf, recProf, destProf, (Vector2){ 0, 0 }, 0.0f, WHITE);

    // aluno: um quadro do spritesheet (4x3); fica apagado enquanto o professor fala
    float qL = jogo->texturaJogador.width / 4.0f;
    float qA = jogo->texturaJogador.height / 3.0f;
    Rectangle recAluno = { 0, qA, qL, qA };
    Color corAluno = (jogo->estadoFeedback == FEEDBACK_NENHUM) ? GRAY : WHITE;
    DrawTexturePro(jogo->texturaJogador, recAluno, destAluno, (Vector2){ 0, 0 }, 0.0f, corAluno);

    // caixa de diálogo e margens do texto
    DrawRectangleRounded(caixa, 0.1f, 8, Fade(BLACK, 0.85f));
    DrawRectangleRoundedLinesEx(caixa, 0.1f, 8, 2.0f, RAYWHITE);
    int mx = (int)caixa.x + escalar(20, e);
    int my = (int)caixa.y + escalar(16, e);
    int rodape = (int)(caixa.y + caixa.height) - escalar(28, e);

    if (jogo->estadoFeedback == FEEDBACK_NENHUM) {
        // nome do professor e pergunta
        DrawText(TextFormat("%s:", jogo->professor.nome), mx, my, fonteQuiz(20, e), YELLOW);
        DrawText(TextSubtext(pergunta, 0, letras), mx, my + escalar(34, e), fonteQuiz(18, e), WHITE);

        if (terminou) {
            // Mostra a instrução enquanto aguarda resposta
            DrawText("Pressione [A], [B], [C] ou [D]", mx, rodape, fonteQuiz(14, e), LIGHTGRAY);

            // verifica a resposta do teclado
            const char *respostaJogador = NULL;
            if (IsKeyPressed(KEY_A)) { respostaJogador = "A"; jogo->respostaEscolhida = 'A'; }
            if (IsKeyPressed(KEY_B)) { respostaJogador = "B"; jogo->respostaEscolhida = 'B'; }
            if (IsKeyPressed(KEY_C)) { respostaJogador = "C"; jogo->respostaEscolhida = 'C'; }
            if (IsKeyPressed(KEY_D)) { respostaJogador = "D"; jogo->respostaEscolhida = 'D'; }

            if (respostaJogador != NULL) {
                jogo->tempoInicioFeedback = GetTime();

                // compara a resposta com o gabarito
                if (strcmp(respostaJogador, jogo->professor.resposta) == 0) {
                    jogo->estadoFeedback = FEEDBACK_ACERTOU;
                    concluir_ajuda_professor(&jogo->jogador, &jogo->professor);
                } else {
                    jogo->estadoFeedback = FEEDBACK_ERROU;
                }
            }
        } else {
            // enquanto a pergunta ainda está sendo digitada
            DrawText("[Espaco] pular", mx, rodape, fonteQuiz(14, e), LIGHTGRAY);
        }
    } else {
        // Renderização visual do Feedback (Acertou / Errou): o aluno responde e o professor reage
        DrawText("Aluno:", mx, my, fonteQuiz(20, e), SKYBLUE);
        DrawText(TextFormat("é... eu acho que é %c", jogo->respostaEscolhida), mx, my + escalar(30, e), fonteQuiz(18, e), WHITE);
        DrawText("Prof:", mx, my + escalar(80, e), fonteQuiz(20, e), YELLOW);

        if (jogo->estadoFeedback == FEEDBACK_ACERTOU) {
            if(jogo->professor.tipo == PROFESSOR_1){
                DrawText("Isso mesmo! Aqui esta o pendrive com \numa parte do seu projeto.\nMas cuidado! os robos estão \nchegando...tome essa \npilha para sua lanterna!", mx, my + escalar(110, e), fonteQuiz(18, e), GREEN);
            }else if(jogo->professor.tipo == PROFESSOR_2){
                DrawText("Muito bem! toma aqui seu pendrive \nMas cuidado! Mineiro configurou muito bem esses robôs \n e eles tão comendo as \nbaterias restantes\ntome essa \npilha para sua lanterna!", mx, my + escalar(110, e), fonteQuiz(18, e), GREEN);
            }else if(jogo->professor.tipo == PROFESSOR_3){
                DrawText("Perfeito! Pegue seu ultimo pendrive! \nCorra logo para a saída...\neles estão cada vez mais perigosos\ntome essa \npilha para sua lanterna!", mx, my + escalar(110, e), fonteQuiz(18, e), GREEN);
            }
        } else {
            if(jogo->professor.tipo == PROFESSOR_1){
                DrawText("Ta errado...se tivesse feito \nas questoes do EdenAI teria \nacertado. \nTente de Novo!", mx, my + escalar(110, e), fonteQuiz(18, e), RED);
            }else if(jogo->professor.tipo == PROFESSOR_2){
                DrawText("Errasse cara, cuidado com a prova...", mx, my + escalar(110, e), fonteQuiz(18, e), RED);
            }else if(jogo->professor.tipo == PROFESSOR_3){
                DrawText("Eh... tá errado alguem tem que ver isso aí.", mx, my + escalar(110, e), fonteQuiz(18, e), RED);
            }
        }

        // conta um tempo pra voltar pra tela inicial
        if (GetTime() - jogo->tempoInicioFeedback >= 1.5) {
            if (jogo->estadoFeedback == FEEDBACK_ACERTOU) { 
                jogo->telaAtual = TELA_NORMAL;
            }
            jogo->estadoFeedback = FEEDBACK_NENHUM;
            jogo->respostaEscolhida = '\0';
        }
    }
}

// entrega o pendrive ao jogador, recarrega a bateria e muda a cor do professor
void concluir_ajuda_professor(Jogador *jogador, Professor *professor) {
    if (jogador == NULL || professor == NULL || professor->ajudado) return;

    professor->ajudado = true;
    jogador->temPendrive = true;
    jogador->bateria = 100;
}
