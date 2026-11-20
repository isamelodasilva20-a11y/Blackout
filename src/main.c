#include <stdlib.h>
#include "raylib.h"
#include <time.h>

#include "estrutura.h"
#include "jogador.h"
#include "mapa.h"
#include "memoria.h"
#include "itens.h"
#include "professor.h"
#include "render3d.h"
#include "hud.h"
#include "robos.h"
#include "ranking.h"


// Volta a partida para o comeco (andar 1, vidas cheias, cronometro zerado).
// Usada pela tecla R e pelo clique em "TENTAR DE NOVO" no game over.
static void reiniciarPartida(Jogo *jogo) {
    jogo->jogador.vidas = VIDAS_INICIAIS;
    jogo->jogador.bateria = 0;
    jogo->jogador.tempoLanterna = 0.0f;
    jogo->jogador.tempoInvulneravel = 0.0f;
    jogo->jogador.tempoCurtoCircuito = 0.0f;
    jogo->tempoJogo = 0.0f;
    jogo->telaAtual = TELA_NORMAL;
    mapaCarregarAndar(jogo, 1);
}


int main(void) {
    srand(time(NULL)); 
    SetConfigFlags(FLAG_WINDOW_RESIZABLE); //permite alterar o tamanho da janela dinamicamente
    InitWindow(LARGURA_BASE, ALTURA_BASE, "Blackout"); //define a janela com altura e largura especifica assim como o titulo da janela
    SetExitKey(KEY_NULL); //desativa o botão ESC como fechador de janela, pois usamos como pause
    SetWindowMinSize(LARGURA_BASE / 2, ALTURA_BASE / 2);//define o menor tamanho de janela possivel
    SetTargetFPS(60); //define o fps

    //identifica o tipo de monitor pra se adequar
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
    
    // Carregamento de imagens

    //jogador
    jogo.texturaJogador = LoadTexture("imagens/jogador.png");
    
    // Prof. Thiaguinho
    jogo.texturasProfessores[PROFESSOR_1][0] = LoadTexture("imagens/professor_1_normal.png");
    jogo.texturasProfessores[PROFESSOR_1][1] = LoadTexture("imagens/professor_1_joinha.png");
    
    // Prof. Daniel
    jogo.texturasProfessores[PROFESSOR_2][0] = LoadTexture("imagens/professor_2_normal.png");
    jogo.texturasProfessores[PROFESSOR_2][1] = LoadTexture("imagens/professor_2_joinha.png");
    
    // Prof. Péricles
    jogo.texturasProfessores[PROFESSOR_3][0] = LoadTexture("imagens/professor_3_normal.png");
    jogo.texturasProfessores[PROFESSOR_3][1] = LoadTexture("imagens/professor_3_joinha.png");
    
    //baterias
    jogo.texturaItem = LoadTexture("imagens/bateria.png");
    
    // Aplica o filtro Point em todas as texturas para manter o visual Pixel Art nítido (24x24)
    SetTextureFilter(jogo.texturaJogador, TEXTURE_FILTER_POINT);
    SetTextureFilter(jogo.texturaItem, TEXTURE_FILTER_POINT);
    for (int p = 0; p < 3; p++) {
        for (int pose = 0; pose < 2; pose++) {
            SetTextureFilter(jogo.texturasProfessores[p][pose], TEXTURE_FILTER_POINT);
        }
    }
    roboCarregar(&jogo);

    jogo.jogador.vidas = VIDAS_INICIAIS;
    jogo.jogador.bateria = 0;
    jogo.jogador.tempoInvulneravel = 0.0f;

    int carregou_mapa = mapaCarregarAndar(&jogo, 1);

    float tempoInativo = 0.0f;
    float tempoPasso = 0.0f;
    const float INTERVALO_PASSO = 0.20f;
    int venceu = 0;
    int pausado = 0;

    EntradaRanking listaRanking[MAX_RANKING];
    int totalRanking = 0;

    // "sair" vira true quando o jogador escolhe sair (ESC na tela inicial,
    // ESC ou SAIR na vitoria). O laco principal termina e a limpeza do final
    // roda normalmente. (Na pausa e no game over, MENU volta para a tela
    // inicial; de la, ESC sai.)
    bool sair = false;
    if (carregou_mapa) sair = !telaInicialMostrar(jogo.jogador.nome, 32);

    InitAudioDevice();
    Music musicaFundo = LoadMusicStream("imagens/musica/running_out_of_time.ogg");
    musicaFundo.looping = true;
    SetMusicVolume(musicaFundo, 0.45f);
    PlayMusicStream(musicaFundo);

    // Instante em que a partida comecou, para o efeito da luz acendendo.
    // Usa GetTime() (relogio real) e nao a soma de GetFrameTime(): o primeiro
    // quadro inclui o tempo de carregar a musica e pularia a animacao.
    double inicioLuz = GetTime();

    // Pedido de voltar ao menu (Q ou MENU na pausa, ESC ou MENU no game over). O menu so e aberto
    // no fim do quadro, depois do EndDrawing: a tela inicial tem o proprio
    // laco de desenho e nao pode rodar dentro de um BeginDrawing aberto.
    bool voltarMenu = false;

    while (carregou_mapa && !sair && !WindowShouldClose()) {
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

        // --- P ou ESC pausa e continua (so durante o jogo, nao no quiz nem no fim) ---
        // As duas teclas ficam neste unico lugar: se a tela de pausa tambem
        // tratasse o ESC, a pausa abriria e fecharia no mesmo quadro.
        if ((IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) && !venceu && jogo.telaAtual == TELA_NORMAL) {
            pausado = !pausado;
        } else if (pausado && IsKeyPressed(KEY_Q)) {
            // --- Q na pausa: voltar ao menu inicial (o menu abre no fim do quadro) ---
            voltarMenu = true;
        }

        // --- ESC no game over: voltar ao menu inicial; na vitoria: sair do jogo ---
        if (jogo.telaAtual == TELA_GAME_OVER && IsKeyPressed(KEY_ESCAPE)) {
            voltarMenu = true;
        } else if (venceu && IsKeyPressed(KEY_ESCAPE)) {
            sair = true;
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
                        carregou_mapa = 0;
                        break;
                    }
                } else {
                    venceu = 1;
                    rankingSalvar(jogo.jogador.nome, jogo.tempoJogo);
                    totalRanking = rankingCarregar(listaRanking, MAX_RANKING);
                }
            }
        }

        // A câmera 3D desliza até a casa/direção nova do jogador
        if (jogo.modoVisao == 1) {
            render3dAtualizarCamera(&jogo, GetFrameTime());
        }

        if (jogo.telaAtual == TELA_QUIZ) {
            jogo.tempoFala += GetFrameTime();
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

            // Luz acendendo no comeco da partida: por cima do jogo e do HUD,
            // mas embaixo das telas de quiz, pausa e game over
            desenharLuzAcendendo((float)(GetTime() - inicioLuz));

            if (jogo.telaAtual == TELA_QUIZ) desenhar_quiz(&jogo);

            // As telas abaixo devolvem o botao clicado com o mouse (AcaoTela);
            // as teclas equivalentes sao lidas aqui mesmo ou no comeco do laco
            if (venceu) {
                if (desenharTelaVitoria(&jogo, listaRanking, totalRanking) == ACAO_SAIR) sair = true;
            }

            if (jogo.telaAtual == TELA_GAME_OVER) {
                AcaoTela acao = desenharTelaGameOver(&jogo);
                if (IsKeyPressed(KEY_R) || acao == ACAO_REINICIAR) {
                    reiniciarPartida(&jogo);
                    inicioLuz = GetTime(); // a luz acende de novo ao recomecar
                } else if (acao == ACAO_MENU) {
                    voltarMenu = true;
                }
            }

            if (pausado) {
                AcaoTela acao = desenharTelaPausa(&jogo);
                if (acao == ACAO_CONTINUAR) pausado = 0;
                else if (acao == ACAO_MENU) voltarMenu = true;
            }

        EndDrawing();

        // --- Voltar ao menu inicial (pedido na pausa ou no game over) ---
        if (voltarMenu && !sair) {
            voltarMenu = false;
            StopMusicStream(musicaFundo);
            reiniciarPartida(&jogo); 
            pausado = 0;

            // O nome digitado antes continua no campo (da para trocar).
            // ENTER: musica do comeco e a luz acende de novo. ESC ou X: sai.
            if (telaInicialMostrar(jogo.jogador.nome, 32)) {
                PlayMusicStream(musicaFundo);
                inicioLuz = GetTime();
            } else {
                sair = true;
            }
        }
    }

    UnloadMusicStream(musicaFundo);
    CloseAudioDevice();

    liberar_todos_itens(&jogo.itens);
    robosLiberar(&jogo.robos);
    mapaLiberar(&jogo.mapa);

    memRelatorio();
    UnloadTexture(jogo.texturaJogador);
    UnloadTexture(jogo.texturaItem);

    for (int p = 0; p < 3; p++) {
        for (int pose = 0; pose < 2; pose++) {
            UnloadTexture(jogo.texturasProfessores[p][pose]);
        }
    }
    roboDescarregar(&jogo);

    CloseWindow();
    return carregou_mapa ? 0 : 1;
}