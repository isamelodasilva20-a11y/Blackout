#include "raylib.h"
#include "jogo.h"
#include "jogador.h"
#include "mapa.h"
#include "memoria.h"
#include "itens.h"
#include "professor.h"
#include "render3d.h"
#include "hud.h"
#include "robos.h"
#include "estrutura.h"

int main(void) {
    // Janela redimensionavel: o jogador pode arrastar a borda ou maximizar,
    // e tudo (mapa, 3D, HUD e quiz) se ajusta ao novo tamanho
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(LARGURA_BASE, ALTURA_BASE, "Blackout");
    SetWindowMinSize(LARGURA_BASE / 2, ALTURA_BASE / 2);
    SetTargetFPS(60);

    // Se o monitor for pequeno (ex.: notebook com zoom), a janela comeca
    // menor, ocupando 90% da tela e mantendo a proporcao 1024x640
    int monitor = GetCurrentMonitor();
    int larguraMonitor = GetMonitorWidth(monitor);
    int alturaMonitor = GetMonitorHeight(monitor);
    float cabeX = 0.9f * (float)larguraMonitor / LARGURA_BASE;
    float cabeY = 0.9f * (float)alturaMonitor / ALTURA_BASE;
    float encolher = (cabeX < cabeY) ? cabeX : cabeY;
    if (encolher > 0.0f && encolher < 1.0f) {
        int largura = (int)(LARGURA_BASE * encolher);
        int altura = (int)(ALTURA_BASE * encolher);
        SetWindowSize(largura, altura);
        SetWindowPosition((larguraMonitor - largura) / 2, (alturaMonitor - altura) / 2);
    }

    Jogo jogo = {0};
    
    // Carregamento do jogador
    jogo.texturaJogador = LoadTexture("imagens/jogador.png");
    SetTextureFilter(jogo.texturaJogador, TEXTURE_FILTER_POINT);

    // --- PONTO 3: Carregamento das texturas dos 3 Professores (Normal e Joinha) ---
    // Prof. Thiaguinho (ID: 0)
    jogo.texturasProfessores[PROFESSOR_1][0] = LoadTexture("imagens/professor_1_normal.png");
    jogo.texturasProfessores[PROFESSOR_1][1] = LoadTexture("imagens/professor_1_joinha.png");

    // Prof. Daniel (ID: 1)
    jogo.texturasProfessores[PROFESSOR_2][0] = LoadTexture("imagens/professor_2_normal.png");
    jogo.texturasProfessores[PROFESSOR_2][1] = LoadTexture("imagens/professor_2_joinha.png");

    // Prof. Péricles (ID: 2)
    jogo.texturasProfessores[PROFESSOR_3][0] = LoadTexture("imagens/professor_3_normal.png");
    jogo.texturasProfessores[PROFESSOR_3][1] = LoadTexture("imagens/professor_3_joinha.png");

    jogo.texturaItem = LoadTexture("imagens/bateria.png");

    // Aplica o filtro Point em todas as texturas para manter o visual Pixel Art nítido (24x24)
    for (int p = 0; p < 3; p++) {
        for (int pose = 0; pose < 2; pose++) {
            SetTextureFilter(jogo.texturasProfessores[p][pose], TEXTURE_FILTER_POINT);
        }
    }

    // --- CARREGAMENTO DOS ROBÔS ---
    // --- CARREGAMENTO DOS ROBÔS E RECURSOS DO JOGO ---
    jogoCarregar(&jogo);

    jogo.jogador.vidas = VIDAS_INICIAIS; // as vidas valem para a partida toda, nao por andar

    // Começa no andar 1. O professor (posição e pergunta) também vem daqui.
    // Se der erro, o loop nem roda, mas o programa ainda passa pela limpeza
    // do final (memória, textura e janela) antes de sair.
    int carregou = mapaCarregarAndar(&jogo, 1);

    float acumuladorTempo = 0.0f;
    float tempoInativo = 0.0f;
    float tempoPasso = 0.0f; // Temporizador para controlar a velocidade do movimento contínuo
    const float INTERVALO_PASSO = 0.20f; // Intervalo de 0.20s entre cada passo ao segurar a tecla
    int venceu = 0;


    while (carregou && !WindowShouldClose()) {
        // --- F11 alterna tela cheia (sem borda, na resolucao do monitor) ---
        if (IsKeyPressed(KEY_F11)) {
            ToggleBorderlessWindowed();
        }

        // --- Cronometro: soma o tempo de cada quadro ate o jogador vencer ---
        if (!venceu) {
            jogo.tempoJogo += GetFrameTime();
        }

        // --- V alterna a visão: 0 = 2D (de cima), 1 = 3D (primeira pessoa) ---
        if (IsKeyPressed(KEY_V)) {
            jogo.modoVisao = !jogo.modoVisao;
            // Ao entrar no 3D a câmera já começa na casa do jogador, sem animação
            if (jogo.modoVisao == 1) render3dReiniciarCamera(&jogo);
        }

        // --- Professor ---
        // Testado ANTES de ler as teclas: assim o quiz só abre no quadro seguinte
        // ao passo, e a tecla usada para andar até o professor (A ou D) não é
        // lida pelo quiz como resposta.
        if (!venceu && jogo.telaAtual == TELA_NORMAL &&
            verificar_interacao_professor(&jogo.jogador, &jogo.professor)) {
            jogo.telaAtual = TELA_QUIZ;
        }

        if (!venceu && jogo.telaAtual == TELA_NORMAL) {
            tempoPasso += GetFrameTime();

            // Atualizacao continua e fluida de todos os robos (estilo Pac-Man)
            robosAtualizarContinuo(&jogo, GetFrameTime());
            checarVitoriaDerrotaRobos(&jogo);
         
            if (jogo.modoVisao == 0) {
                // --- Controles 2D Contínuos (na main.c) ---
                int tentandoMover = (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) ||
                                    IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN) ||
                                    IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT) ||
                                    IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT));

                if (tentandoMover) {
                    tempoInativo = 0.0f;

                    if (tempoPasso >= INTERVALO_PASSO) {
                        tempoPasso = 0.0f;
                        // Guarda o quadro antes do passo: o movimentar_jogador tambem mexe nele
                        int quadroAnterior = jogo.jogador.frame;

                        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
                            jogo.jogador.direcao = DIR_CIMA;
                            movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_CIMA);
                        }
                        else if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
                            jogo.jogador.direcao = DIR_BAIXO;
                            movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_BAIXO);
                        }
                        else if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
                            jogo.jogador.direcao = DIR_ESQUERDA;
                            movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_ESQUERDA);
                        }
                        else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
                            jogo.jogador.direcao = DIR_DIREITA;
                            movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_DIREITA);
                        }

                        // Alterna entre o quadro 0 (parado) e o 1 (dando o passo). Na
                        // folha de sprites as colunas 2 e 3 do perfil olham para o
                        // lado contrario, e usa-las fazia o boneco andar de costas.
                        jogo.jogador.frame = (quadroAnterior == 0) ? 1 : 0;
                    }
                } else {
                    jogo.jogador.frame = 0;
                    tempoInativo += GetFrameTime();

                    if (tempoInativo >= 0.5f) {
                        jogo.jogador.direcao = DIR_BAIXO;
                    }
                }
            } else {
                // --- Controles 3D Contínuos ---
                if (tempoPasso >= INTERVALO_PASSO) {
                    int seMoveu3D = 0;

                    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
                        movimentar_jogador(&jogo.jogador, &jogo.mapa, jogo.jogador.direcao);
                        seMoveu3D = 1;
                    }
                    else if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
                        recuar_jogador(&jogo.jogador, &jogo.mapa);
                        seMoveu3D = 1;
                    }
                    else if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
                        girar_jogador(&jogo.jogador, false);
                        seMoveu3D = 1;
                    }
                    else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
                        girar_jogador(&jogo.jogador, true);
                        seMoveu3D = 1;
                    }

                    if (seMoveu3D) {
                        tempoPasso = 0.0f;
                    }
                }
            }


            // Daqui para baixo vale para os dois modos: a lógica é a mesma
            coletar_item(&jogo.itens, &jogo.jogador);

            // --- Troca de andar ---
            int naEscada = (jogo.jogador.posicao.linha == jogo.mapa.saida.linha &&
                            jogo.jogador.posicao.coluna == jogo.mapa.saida.coluna);

            // Muda de andar apenas se o professor foi ajudado E o jogador subiu na escada
            if (jogo.professor.ajudado && naEscada) {
                if (jogo.andarAtual < TOTAL_ANDARES) {
                    if (!mapaCarregarAndar(&jogo, jogo.andarAtual + 1)) {
                        carregou = 0; // erro no próximo andar: sai do loop e vai para a limpeza
                        break;
                    }
                } else {
                    venceu = 1;
                }
            }
        }

        // A câmera 3D desliza até a casa/direção nova do jogador
        if (jogo.modoVisao == 1) render3dAtualizarCamera(&jogo, GetFrameTime());

        acumuladorTempo += GetFrameTime();
        if (acumuladorTempo >= 0.05f) {
            acumuladorTempo -= 0.05f;
        }

        BeginDrawing();
            ClearBackground(BLACK);

            if (jogo.modoVisao == 0) {
                // 2D: o tamanho da casa e a origem sao recalculados todo quadro
                // a partir do tamanho atual da janela (mapa centralizado acima do HUD).
                // Ordem = camadas: piso/paredes, escada, itens, professor e,
                // por último, o jogador (fica por cima de tudo)
                Vector2 origem = { 0.0f, 0.0f };
                int tamanhoCasa = 24;
                hudLayoutMapa(&jogo.mapa, &origem, &tamanhoCasa);
                desenharMapa2D(&jogo.mapa, origem, tamanhoCasa);
                desenharSaida2D(&jogo.mapa, origem, tamanhoCasa, jogo.professor.ajudado); // verde = liberada pelo professor
                desenhar_item(&jogo, origem, tamanhoCasa);
                desenhar_professor(&jogo, origem, tamanhoCasa);
                desenharRobos(&jogo, origem, tamanhoCasa);
                desenharJogador2D(&jogo, origem, tamanhoCasa);
            } else {
                // 3D: cena em primeira pessoa ocupando a janela toda e o minimapa no canto
                desenharCena3D(&jogo);
                desenharMinimapa(&jogo);
                desenharNickProfessor3D(&jogo.professor);
            }

            // HUD por cima de tudo, na faixa de baixo (nos dois modos)
            desenharHUD(&jogo);

            if (jogo.telaAtual == TELA_QUIZ) desenhar_quiz(&jogo);

            if (venceu) desenharTelaVitoria(&jogo);

        EndDrawing();
    }

    // Limpeza: roda sempre, inclusive quando o andar não carregou
    liberar_todos_itens(&jogo.itens);
    mapaLiberar(&jogo.mapa);

    memRelatorio();
    UnloadTexture(jogo.texturaJogador);

    for (int p = 0; p < 3; p++) {
        for (int pose = 0; pose < 2; pose++) {
            UnloadTexture(jogo.texturasProfessores[p][pose]);
        }
    }

    CloseWindow();
    return carregou ? 0 : 1;
}
