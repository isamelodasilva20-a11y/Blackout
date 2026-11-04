#ifndef PROFESSOR_H
#define PROFESSOR_H

#include "jogo.h"

void inicializar_professor(Professor *professor, Posicao posicao, const char *nome, const char *pergunta, const char *resposta, TipoProfessor tipo);

bool verificar_interacao_professor(Jogador *jogador, Professor *professor);

// Inicializa o professor com a pergunta do andar indicado (1, 2, 3...)
void inicializar_professor_andar(Professor *professor, Posicao posicao, int andar);

// origem = canto superior esquerdo do mapa na tela (permite desenhar no minimapa)
void desenhar_professor(const Jogo *jogo, Vector2 origem, int tamanhoCasa);

void concluir_ajuda_professor(Jogador *jogador, Professor *professor);

void desenhar_quiz(Jogo *jogo);

#endif
