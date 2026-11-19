#include <stdlib.h>
#include <time.h>
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
#include "ranking.h"


int main(void) {
    srand(time(NULL));
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

    jogoCarregar(&jogo);

    jogo.jogador.vidas = VIDAS_INICIAIS;
    jogo.jogador.bateria = 0;
    jogo.jogador.tempoInvulneravel = 0.0f;

    // Começa no andar 1. O professor (posição e pergunta) também vem daqui.
    int carregou = mapaCarregarAndar(&jogo, 1);

    float acumuladorTempo = 0.0f;
    float tempoInativo = 0.0f;
    float tempoPasso = 0.0f;
    const float INTERVALO_PASSO = 0.20f;
    int venceu = 0;
    int pausado = 0;

    EntradaRanking listaRanking[MAX_RANKING];
    int totalRanking = 0;

    if (carregou) telaInicialMostrar(jogo.jogador.nome, 32);
    
    InitAudioDevice();
    Music musicaFundo = LoadMusicStream("imagens/musica/running_out_of_time.ogg");
    musicaFundo.looping = true;
    SetMusicVolume(musicaFundo, 0.45f);
    PlayMusicStream(musicaFundo);

    while (carregou && !WindowShouldClose()) {
        // --- Atualiza a musica de fundo e ajusta volume conforme a tela ---
        UpdateMusicStream(musicaFundo);
        if (pausado || jogo.telaAtual == TELA_GAME_OVER || venceu) {
            SetMusicVolume(musicaFundo, 0.15f);
        } else if (jogo.telaAtual == TELA_QUIZ) {
            SetMusicVolume(musicaFundo, 0.25f);
        } else {
            SetMusicVolume(musicaFundo, 0.45f);
        }

        // --- F11 alterna tela cheia (sem borda, na resolucao do monitor) ---
        if (IsKeyPressed(KEY_F11)) {
            ToggleBorderlessWindowed();
        }

        // --- P pausa e continua (so durante o jogo, nao no quiz nem no fim) ---
        if (IsKeyPressed(KEY_P) && !venceu && jogo.telaAtual == TELA_NORMAL) {
            pausado = !pausado;
        }

        // --- Cronometro: soma o tempo de cada quadro ate o jogador vencer ou dar Game Over ---
        if (!venceu && !pausado && jogo.telaAtual != TELA_GAME_OVER) {
            jogo.tempoJogo += GetFrameTime();
        }

        // --- V alterna a visão: 0 = 2D (de cima), 1 = 3D (primeira pessoa) ---
        if (IsKeyPressed(KEY_V)) {
            jogo.modoVisao = !jogo.modoVisao;
            if (jogo.modoVisao == 1) render3dReiniciarCamera(&jogo);
        }

        // --- Professor ---
        if (!venceu && !pausado && jogo.telaAtual == TELA_NORMAL &&
            verificar_interacao_professor(&jogo.jogador, &jogo.professor)) {
            jogo.telaAtual = TELA_QUIZ;
            jogo.tempoFala = 0.0f;
            jogo.respostaEscolhida = '\0';
        }

        if (!venceu && !pausado && jogo.telaAtual == TELA_NORMAL) {
            float dt = GetFrameTime();
            tempoPasso += dt;

            // --- Ativa a lanterna ao apertar L (se tiver bateria e ainda estiver desligada) ---
            if (IsKeyPressed(KEY_L) && jogo.jogador.bateria > 0 && jogo.jogador.tempoLanterna <= 0.0f) {
                jogo.jogador.tempoLanterna = 10.0f;
            }

            // --- Contagem regressiva apenas depois que L foi pressionado ---
            if (jogo.jogador.tempoLanterna > 0.0f) {
                jogo.jogador.tempoLanterna -= dt;
                jogo.jogador.bateria = (int)((jogo.jogador.tempoLanterna / 10.0f) * 100.0f);
                if (jogo.jogador.tempoLanterna <= 0.0f) {
                    jogo.jogador.tempoLanterna = 0.0f;
                    jogo.jogador.bateria = 0;
                }
            }

            if (jogo.jogador.tempoInvulneravel > 0.0f) {
                jogo.jogador.tempoInvulneravel -= dt;
                if (jogo.jogador.tempoInvulneravel < 0.0f) {
                    jogo.jogador.tempoInvulneravel = 0.0f;
                }
            }

            // Desconta o tempo do curto-circuito (inversao de controles no 3o andar)
            if (jogo.jogador.tempoCurtoCircuito > 0.0f) {
                jogo.jogador.tempoCurtoCircuito -= dt;
                if (jogo.jogador.tempoCurtoCircuito < 0.0f) {
                    jogo.jogador.tempoCurtoCircuito = 0.0f;
                }
            }

            // Atualizacao continua e fluida de todos os robos
            robosAtualizarContinuo(&jogo, dt);
            checarVitoriaDerrotaRobos(&jogo);
         
            if (jogo.modoVisao == 0) {
                // --- Controles 2D Contínuos ---
                int tentandoMover = (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) ||
                                    IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN) ||
                                    IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT) ||
                                    IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT));

                if (tentandoMover) {
                    tempoInativo = 0.0f;

                    if (tempoPasso >= INTERVALO_PASSO) {
                        tempoPasso = 0.0f;
                        int quadroAnterior = jogo.jogador.frame;

                        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
                            movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_CIMA);
                        }
                        else if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
                            movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_BAIXO);
                        }
                        else if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
                            movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_ESQUERDA);
                        }
                        else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
                            movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_DIREITA);
                        }

                        jogo.jogador.frame = (quadroAnterior == 0) ? 1 : 0;
                    }
                } else {
                    jogo.jogador.frame = 0;
                    tempoInativo += dt;

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

            // Coleta de itens e troca de andar
            coletar_item(&jogo.itens, &jogo.jogador);

            int naEscada = (jogo.jogador.posicao.linha == jogo.mapa.saida.linha &&
                            jogo.jogador.posicao.coluna == jogo.mapa.saida.coluna);

            if (jogo.professor.ajudado && naEscada) {
                if (jogo.andarAtual < TOTAL_ANDARES) {
                    if (!mapaCarregarAndar(&jogo, jogo.andarAtual + 1)) {
                        carregou = 0;
                        break;
                    }
                } else {
                    venceu = 1;
                    rankingSalvar(jogo.jogador.nome, jogo.tempoJogo);
                    totalRanking = rankingCarregar(listaRanking, MAX_RANKING);
                }
            }
        } // <--- Chave que estava faltando!

        // A câmera 3D desliza até a casa/direção nova do jogador
        if (jogo.modoVisao == 1) {
            render3dAtualizarCamera(&jogo, GetFrameTime());
        }

        if (jogo.telaAtual == TELA_QUIZ) {
            jogo.tempoFala += GetFrameTime();
        }
        
        acumuladorTempo += GetFrameTime();
        if (acumuladorTempo >= 0.05f) {
            acumuladorTempo -= 0.05f;
        }

        BeginDrawing();
            ClearBackground(BLACK);

            if (jogo.modoVisao == 0) {
                Vector2 origem = { 0.0f, 0.0f };
                int tamanhoCasa = 24;
                hudLayoutMapa(&jogo.mapa, &origem, &tamanhoCasa);
                desenharMapa2D(&jogo.mapa, origem, tamanhoCasa);
                desenharSaida2D(&jogo.mapa, origem, tamanhoCasa, jogo.professor.ajudado);
                desenhar_item(&jogo, origem, tamanhoCasa);
                desenhar_professor(&jogo, origem, tamanhoCasa);
                desenharRobos(&jogo, origem, tamanhoCasa);
                desenharJogador2D(&jogo, origem, tamanhoCasa);
            } else {
                desenharCena3D(&jogo);
                desenharMinimapa(&jogo);
                desenharNickProfessor3D(&jogo.professor);
            }

            // HUD por cima de tudo, na faixa de baixo (nos dois modos)
            desenharHUD(&jogo);

            if (jogo.telaAtual == TELA_QUIZ) desenhar_quiz(&jogo);

            if (venceu) desenharTelaVitoria(&jogo, listaRanking, totalRanking);

            if (jogo.telaAtual == TELA_GAME_OVER) {
                desenharTelaGameOver();
                if (IsKeyPressed(KEY_R)) {
                    jogo.jogador.vidas = VIDAS_INICIAIS;
                    jogo.jogador.bateria = 0;
                    jogo.jogador.tempoLanterna = 0.0f;
                    jogo.jogador.tempoInvulneravel = 0.0f;
                    jogo.jogador.tempoCurtoCircuito = 0.0f;
                    jogo.jogador.temPendrive = false;
                    jogo.tempoJogo = 0.0f;
                    jogo.telaAtual = TELA_NORMAL;
                    mapaCarregarAndar(&jogo, 1);
                }
            }

            if (pausado) desenharTelaPausa();

        EndDrawing();
    }

    // Limpeza: roda sempre, inclusive quando o andar não carregou
    UnloadMusicStream(musicaFundo);
    CloseAudioDevice();

    liberar_todos_itens(&jogo.itens);
    robosLiberar(&jogo.robos);
    mapaLiberar(&jogo.mapa);

    memRelatorio();
    UnloadTexture(jogo.texturaJogador);

    for (int p = 0; p < 3; p++) {
        for (int pose = 0; pose < 2; pose++) {
            UnloadTexture(jogo.texturasProfessores[p][pose]);
        }
    }
    jogoDescarregar(&jogo);

    CloseWindow();
    return carregou ? 0 : 1;
}