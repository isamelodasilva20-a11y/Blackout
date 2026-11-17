# Blackout

A CESAR School foi invadida por hackers e a energia do prédio está falhando. Os arquivos do seu TCC sumiram dos servidores, e só os professores da cadeira de PIF têm os backups. Para recuperá-los, você precisa atravessar os três andares do prédio, encontrar o professor de cada andar, responder à pergunta dele e subir pela escada. No caminho, os robôs do professor Mineiro, modificados pelos hackers, tentam te pegar e destruir os arquivos.

## Como jogar

Cada andar é um labirinto de corredores e salas. O objetivo em todos é o mesmo:

1. Encontre o professor do andar (marcado com um `!` amarelo) e pare na casa dele.
2. Abre um quiz com uma pergunta de múltipla escolha. Se errar, é só tentar de novo. Ao acertar, você recebe o pendrive e a sua bateria é recarregada.
3. A escada do andar, que antes estava cinza, fica verde. Vá até ela para subir.

Depois do terceiro andar, aparece a tela de vitória com o tempo total da partida.

**Baterias e lanterna.** Há baterias espalhadas pelos andares. Você só consegue pegar uma quando a carga atual está zerada. Cada bateria enche a carga e acende a lanterna por 10 segundos, e a carga vai caindo nesse tempo. Enquanto a lanterna está acesa, os robôs vermelho e laranja fogem de você, ficam azulados e andam mais devagar.

**Vidas.** Você começa com 3 corações. Se um robô vermelho ou laranja encostar em você e houver carga na bateria, a carga é gasta no lugar de um coração. Sem carga, você perde um coração. Depois de cada batida, você fica um instante invulnerável. Sem corações, é game over.

**Game over** acontece quando os corações acabam ou quando o robô roxo alcança o professor antes de você responder ao quiz. Na tela de game over, `R` reinicia a partida a partir do andar 1.

**Robôs.** Todos andam pelo caminho mais curto do labirinto (busca em largura), desviando de paredes e móveis. A cada andar eles ficam um pouco mais rápidos.

| Robô | Comportamento |
|---|---|
| Vermelho (perseguidor) | Vai direto atrás de você. |
| Laranja (interceptador) | Mira duas casas à frente da direção em que você está andando, para te cercar. Quando está a até duas casas de você, vai direto em você. |
| Roxo (temporizador) | Ignora você e vai até o professor. Se chegar lá antes de você responder ao quiz, é game over. Se você encostar nele com carga na bateria, ele fica parado por 5 segundos e a carga é gasta. |

**Tela.** A barra de baixo mostra os corações, o andar, a carga da bateria, o pendrive, a lanterna, o cronômetro e o objetivo atual. Quando um robô chega a 8 casas de você, um alerta de proximidade pisca no topo da tela, primeiro em amarelo, depois em laranja e, bem perto, em vermelho. O jogo tem duas visões: o mapa visto de cima (2D) e a primeira pessoa (3D), com um minimapa no canto.

## Controles

| Onde | Tecla | Ação |
|---|---|---|
| Tela inicial | `Enter` ou `Espaço` | Começar o jogo |
| Visão 2D | `W` `A` `S` `D` ou setas | Andar (segurar a tecla continua andando) |
| Visão 3D | `W` ou seta para cima | Andar para a frente |
| Visão 3D | `S` ou seta para baixo | Andar para trás |
| Visão 3D | `A` / `D` ou setas laterais | Girar 90 graus para a esquerda / direita |
| Durante o jogo | `V` | Alternar entre a visão 2D e a 3D |
| Durante o jogo | `P` | Pausar e continuar |
| Quiz | `A` `B` `C` `D` | Responder |
| Game over | `R` | Recomeçar do andar 1 |
| Qualquer tela | `F11` | Tela cheia |
| Qualquer tela | `Esc` | Fechar o jogo |

## Como compilar e rodar

O jogo usa C99 e a biblioteca [raylib](https://www.raylib.com/).

### Windows

1. Baixe o **w64devkit** (versão x64) e descompacte em `C:\w64devkit`.
2. Adicione `C:\w64devkit\bin` ao `Path` do Windows (Variáveis de ambiente). Se já houver um MinGW antigo no `Path`, o `C:\w64devkit\bin` precisa vir antes dele. Senão, o `gcc` e o `make` errados são usados.
3. Baixe o raylib no arquivo `raylib-*_win64_mingw-w64.zip` (página de releases do raylib no GitHub) e descompacte de modo que existam as pastas `C:\raylib\include` e `C:\raylib\lib`.
4. Abra um terminal novo e confira com `gcc --version` e `make --version`.

Se o raylib estiver em outra pasta, informe o caminho ao compilar: `make RAYLIB_PATH=D:/minhapasta/raylib`.

### macOS

```
xcode-select --install
```

Instale o [Homebrew](https://brew.sh/) e depois:

```
brew install raylib
```

### Comandos

Rode na pasta do projeto:

| Comando | O que faz |
|---|---|
| `make` | Compila o jogo em `build/` |
| `make run` | Compila e abre o jogo |
| `make clean` | Apaga a pasta `build/` |

O jogo carrega as pastas `imagens/` e `andares/` a partir da pasta atual. Por isso, rode-o sempre a partir da raiz do projeto (o `make run` já faz isso).

Ao fechar o jogo, o terminal mostra `Blocos nao liberados: N`. Esse é o contador do `memoria.c`, e o esperado é `0`.

## Estrutura do projeto

| Pasta | Conteúdo |
|---|---|
| `src/` | Código-fonte (`.c`) |
| `include/` | Cabeçalhos (`.h`). As structs do jogo ficam em `estrutura.h` |
| `andares/` | Um arquivo de texto por andar: `andar1.txt`, `andar2.txt` e `andar3.txt` |
| `imagens/` | Sprites do jogador, dos professores, dos robôs e da bateria |
| `build/` | Gerada pelo `make` (objetos e executável) |

Os arquivos de `src/`:

| Arquivo | Responsabilidade |
|---|---|
| `main.c` | Janela, laço principal, leitura das teclas, troca de andar, pausa e limpeza ao sair |
| `mapa.c` | Leitura dos arquivos de andar, matriz do mapa e desenho do mapa em 2D |
| `render3d.c` | Visão 3D em primeira pessoa e minimapa |
| `hud.c` | Barra de informações, alerta de proximidade e telas de vitória, pausa e game over |
| `telainicial.c` | Tela inicial com a história |
| `jogador.c` | Movimento, giro na visão 3D e desenho do jogador |
| `itens.c` | Lista de baterias: criação, coleta, desenho e liberação |
| `professor.c` | Professores de cada andar, perguntas e quiz |
| `robos.c` | Lista de robôs: caminho, fuga, colisões e desenho |
| `jogo.c` | Carrega e descarrega as texturas dos robôs |
| `memoria.c` | `memAlocar` e `memLiberar`, com contador de blocos alocados |

### Formato do arquivo de andar

A primeira linha tem o tamanho do mapa (`linhas colunas`; todos os andares hoje são `25 40`). Depois vem o desenho do mapa, um caractere por casa:

| Caractere | Significado |
|---|---|
| `#` | Parede |
| `.` | Piso de corredor |
| ` ` (espaço) | Piso de sala |
| `=` | Tapete com a logo do CESAR (andável). Um retângulo de `=` vira um tapete só |
| `M` | Mesa (bloqueia a passagem) |
| `C` | Computador (bloqueia a passagem) |
| `P` | Onde o jogador começa no andar |
| `S` | Escada para o próximo andar |
| `I` | Bateria |
| `F` | Professor do andar (obrigatório: um andar sem `F` não carrega) |
| `R` | Robô roxo (temporizador) |
| `X` | Robô vermelho (perseguidor) |
| `Z` | Robô laranja (interceptador) |

As letras de objetos (`P`, `S`, `I`, `F`, `R`, `X`, `Z`) viram piso no mapa, copiando o piso da casa à esquerda (corredor ou sala). Qualquer outro caractere também vira piso. Os andares se encaixam: a escada (`S`) de um andar fica na mesma posição do início (`P`) do andar seguinte.

## Conceitos da disciplina

| Conceito | Onde aparece |
|---|---|
| Structs | `include/estrutura.h`: `Posicao`, `Mapa`, `Jogador`, `Item`, `Professor`, `Robo` e `Jogo`, que reúne o estado da partida inteira |
| Ponteiros | `Jogo *` passado para quase todas as funções. Ponteiro para ponteiro para alterar o início de uma lista: `adicionar_item(Item **, ...)` e `coletar_item` em `itens.c`, `robosAdicionar(Robo **, ...)` e `robosLiberar` em `robos.c`. A matriz do mapa é um `char **` |
| Alocação dinâmica | `memoria.c`: `memAlocar` e `memLiberar` envolvem `malloc` e `free` e contam os blocos, e `memRelatorio` mostra o saldo ao sair. São usadas em `mapa.c` (`mapaCarregar`, `mapaLiberar`) e `itens.c` (`adicionar_item`, `liberar_item`). Os robôs usam `malloc` e `free` direto, em `robosAdicionar` e `robosLiberar` (`robos.c`) |
| Listas encadeadas | Baterias: `Item` com o campo `next`; `adicionar_item` insere no início, `coletar_item` remove o item pego e `liberar_todos_itens` apaga a lista (`itens.c`). Robôs: `Robo` com `next`; `robosAdicionar` insere, `robosAtualizarContinuo` e `desenharRobos` percorrem e `robosLiberar` apaga (`robos.c`) |
| Matrizes | O mapa é uma matriz `char **` alocada linha por linha em `mapaCarregar` e liberada em `mapaLiberar` (`mapa.c`). A busca de caminho dos robôs usa as matrizes `visitado` e `pai` em `calcularProximoPassoBFS` (`robos.c`) |
| Leitura de arquivo | `mapaCarregar` (`mapa.c`) abre o andar com `fopen`, lê o tamanho com `fscanf`, lê cada linha com `fgets` e fecha com `fclose`. `mapaCarregarAndar` monta o nome do arquivo (`andares/andarN.txt`) com `snprintf` |

## Equipe

- Miguel Magalhães Drechsler
- Carlos Vinicius
- Isabela Melo da Silva

Programação Imperativa e Funcional, CESAR School, 2026.2.
