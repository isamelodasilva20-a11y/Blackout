#include "raylib.h"
#include "jogo.h"
#include "jogador.h"
#include "mapa.h"
#include "memoria.h"
#include "itens.h"
#include "professor.h"
#include "render3d.h"
#include "hud.h"

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

    Jogo jogo = {0}; // estado do jogo
    jogo.texturaJogador = LoadTexture("imagens/jogador.png");
    jogo.jogador.vidas = VIDAS_INICIAIS; // as vidas valem para a partida toda, nao por andar

    // Começa no andar 1. O professor (posição e pergunta) também vem daqui.
    // Se der erro, o loop nem roda, mas o programa ainda passa pela limpeza
    // do final (memória, textura e janela) antes de sair.
    int carregou = mapaCarregarAndar(&jogo, 1);

    float acumuladorTempo = 0.0f;
    float tempoInativo = 0.0f;
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
            if (jogo.modoVisao == 0) {
                // --- Controles 2D: cada tecla anda uma casa naquela direção ---
                int seMoveu = 0;
                if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
                    jogo.jogador.direcao = DIR_CIMA; // ex: 0
                    movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_CIMA);
                    seMoveu = 1;
                }
                else if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
                    jogo.jogador.direcao = DIR_BAIXO; // ex: 1
                    movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_BAIXO);
                    seMoveu = 1;
                }
                else if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) {
                    jogo.jogador.direcao = DIR_ESQUERDA; // ex: 2
                    movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_ESQUERDA);
                    seMoveu = 1;
                }
                else if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {
                    jogo.jogador.direcao = DIR_DIREITA; // ex: 3
                    movimentar_jogador(&jogo.jogador, &jogo.mapa, DIR_DIREITA);
                    seMoveu = 1;
                }
                else {
                    jogo.jogador.frame = 0;
                }
                if (seMoveu) {
                    tempoInativo = 0.0f; // Reseta o contador ao se mover
                    // Avança o frame de animação de caminhada (0 a 3)
                    jogo.jogador.frame = (jogo.jogador.frame + 1) % 4;
                } else {
                    tempoInativo += GetFrameTime();

                    // Se ficar 0.5 segundos sem apertar nada, vira para a frente (Idle)
                    if (tempoInativo >= 0.5f) {
                        jogo.jogador.direcao = DIR_BAIXO;
                        jogo.jogador.frame = 0;
                    }
                }
            } else {
                // --- Controles 3D: andam/giram em relação a para onde o jogador olha ---
                // (aqui não tem o "vira para a frente parado", senão a câmera giraria sozinha)
                if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
                    movimentar_jogador(&jogo.jogador, &jogo.mapa, jogo.jogador.direcao); // uma casa para a frente
                }
                else if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
                    recuar_jogador(&jogo.jogador, &jogo.mapa); // uma casa para trás, sem virar
                }
                else if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) {
                    girar_jogador(&jogo.jogador, false); // 90 graus para a esquerda
                }
                else if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {
                    girar_jogador(&jogo.jogador, true);  // 90 graus para a direita
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
                desenhar_item(jogo.itens, origem, tamanhoCasa);
                desenhar_professor(&jogo.professor, origem, tamanhoCasa);
                desenharJogador2D(&jogo, origem, tamanhoCasa);
            } else {
                // 3D: cena em primeira pessoa ocupando a janela toda e o minimapa no canto
                desenharCena3D(&jogo);
                desenharMinimapa(&jogo);
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
    CloseWindow();
    return carregou ? 0 : 1;
}
