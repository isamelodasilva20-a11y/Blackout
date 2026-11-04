#include "robo_interceptador.h"

// Calcula a posição para onde o jogador está a ir (2 casas à frente)
Posicao calcularPosicaoAlvoInterceptador(Jogo *jogo, Robo *robo) {
    Jogador *jogador = &jogo->jogador;
    Posicao alvo = jogador->posicao;
    int distanciaAntecipacao = 2;

    switch (jogador->direcao) {
        case DIR_CIMA:
            alvo.linha -= distanciaAntecipacao;
            break;
        case DIR_BAIXO:
            alvo.linha += distanciaAntecipacao;
            break;
        case DIR_ESQUERDA:
            alvo.coluna -= distanciaAntecipacao;
            break;
        case DIR_DIREITA:
            alvo.coluna += distanciaAntecipacao;
            break;
    }

    // Limites da matriz do mapa
    if (alvo.linha < 0) alvo.linha = 0;
    if (alvo.linha >= jogo->mapa.linhas) alvo.linha = jogo->mapa.linhas - 1;
    if (alvo.coluna < 0) alvo.coluna = 0;
    if (alvo.coluna >= jogo->mapa.colunas) alvo.coluna = jogo->mapa.colunas - 1;

    return alvo;
}

void inicializarRoboInterceptador(RoboInterceptador *robo, Posicao posicaoInicial) {
    robo->base.posicao = posicaoInicial;
    robo->base.posicaoInicial = posicaoInicial;
    robo->base.tipo = ROBO_INTERCEPTADOR;
    robo->base.estado = ESTADO_NORMAL;
    robo->base.next = NULL;
    robo->comportamento = calcularPosicaoAlvoInterceptador;
}

void atualizarRoboInterceptador(Jogo *jogo, RoboInterceptador *robo) {
    if (robo->base.estado == ESTADO_DESLIGADO) return;

    // Obtém o alvo antecipado através do ponteiro de função
    Posicao alvo = robo->comportamento(jogo, (Robo*)robo);
    Posicao proxima = robo->base.posicao;

    // Movimento simples em direção ao ponto de interceptação
    if (proxima.linha < alvo.linha) proxima.linha++;
    else if (proxima.linha > alvo.linha) proxima.linha--;

    if (proxima.coluna < alvo.coluna) proxima.coluna++;
    else if (proxima.coluna > alvo.coluna) proxima.coluna--;

    // Verifica se a próxima posição é válida (não é parede)
    if (jogo->mapa.matriz[proxima.linha][proxima.coluna] != '#') {
        robo->base.posicao = proxima;
    }
}