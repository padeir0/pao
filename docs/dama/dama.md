## Representação do tabuleiro

Bitboards são muito complicadas pra um primeiro programa,
preciso de corretude antes. Performance não é tão importante.

```c
typedef uint8_t Piece 

#define WP (piece)1
#define BP (piece)2
#define WK (piece)3
#define BK (piece)4

typedef struct {
  int x;
  int y;
} Position;

typedef struct {
  Position from;
  Position to;

  bool     capture;
  Position capturedPos;
} HalfMove;

typedef struct {
  HalfMove* list;
  size_t    length
} Move; // with methods to get "start" and "end" positions, "captures" whatever is needed

typedef struct {
  size_t length;
  size_t top;
  Move*  bottom;
} MoveStack; // with all push/pop/clear methods, as usual.

typedef struct {
  Piece pieces[8][8];
  bool  whiteTurn;
  MoveStack history;
  int repeated;
} Game; // with methods like "canMove", "move" and "unmove"
```

Isso é o suficiente pra gerar e verificar os movimentos. O resto é algoritmo.

## Move Generation

Tenho que usar as regras usuais de dama e verificar os numeros de perft com a
sequencia: https://oeis.org/A133046. Vou replicar ela aqui pro futuro:

```
1
7
49
302
1469
7361
36768
179740
845931
3963680
18391564
85242128
388623673
1766623630
7978439499
36263167175
165629569428
758818810990
3493881706141
16114043592799
74545030871553
345100524480819
1602372721738102
7437536860666213
34651381875296000
161067479882075800
752172458688067137
3499844183628002605
16377718018836900735
76309690522352444005
```

Preciso levar em conta a observação:

> Duplicate captures (viz. the situation where a king can capture the same pieces in different directions) are counted separately.

Isso começa a aparecer depois do depth 12, aparentemente.

## Search and evaluation

Vou fazer várias searches diferentes e comparar elas, ao invés de usar
engines externas. Acho que dessa forma eu entendo melhor os drawbacks de cada uma.
Vale ressaltar que eu tive muitos problemas de instabilidade com meu alpha beta search
quando implementei uma engine de xadrez, também tive problemas de verificar o move
generator. Devia ter começado direto com dama kkkkk xadrez é muito complicado.

Sobre evaluation, não sei ao certo, talvez só contagem de peças, dando um valor maior
pra dama, seja o suficiente. Dama é bem dinâmico e quase 100% tático, não preciso
de evaluation posicional que nem no xadrez. Se for necessário levar em conta posição,
posso olhar pra como as peças estão conectadas, principalmente no começo de jogo,
e quão longe uma peça está de se tornar dama.

Sobre a profundidade de pesquisa, só deus sabe... Eu sei que conseguia no máximo uns 4 ou 5 ply
quando programei a engine de xadrez em Go, mas não sei pq era tão pesado assim... Em dama, com
certeza as pesquisas vão ser mais profundas. Também vou programar em C, então devo conseguir uns
50% de performance a mais de graça, já que o código gerado pelo compilador do Go é meio ruinzinho.
Estou esperando ter pelo menos 10 ply, contando 1 ply como um movimento forçado até o final, claro.
Se 10ply não for o suficiente pra um motor decente e tático, eu enfio uma evaluation que leva em conta
as posições.

## Requirements

Quero criar uma interface completamente client-side pra esse motor no navegador.
Quero conseguir pegar um jogo, escrito em alguma variação de PGN, e analisar ele movimento
a movimento: pode ser simplesmente se cada movimento foi a escolha da engine ou não,
não precisa das distinções "brilliant", "excellent", etc. Talvez uma forma de clicar
em um movimento e ver até onde a engine levaria ele. Lembrando que a engine não é perfeita,
então o "melhor movimento" é simplesmente uma escolha dela.

Problemas de interface vou lidar depois, só quero manter eles em mente: preciso de uma lib
pura em C pro Emscripten compilar, não posso ter side effects além de gerenciamento de memória.

Enquanto não tenho a interface, preciso de um jeito de testar a engine, vou fazer isso com
esse PGN que citei, uma interface no terminal e os perfts.

Lidraughts tem uma API que nem o Lichess. Permite o bot jogar e também me permite baixar jogos
de um player. Vou tentar colocar meu bot lá, pelo menos daí eu consigo ter um rating.

link: https://lidraughts.org/api#tag/Games/operation/gamesExportIds

Eles aceitam a versão brasileira de dama, então tá ótimo...

PDN: https://wiegerw.github.io/pdn/pdf/pdn_standard.pdf
