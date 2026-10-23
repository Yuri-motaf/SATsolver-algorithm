# Algoritmo de SAT SOLVER

Consiste em determinar se existe uma atribuição de valores verdadeiros para um conjunto de variáveis booleanas que torne uma fórmula lógica inteira verdadeira (SAT).

## Formato DIMACS

O formato DIMACS CNF é uma representação textual de uma fórmula na forma normal conjuntiva. Uma fórmula na forma normal conjuntiva é uma conjunção (E lógico) de um conjunto de cláusulas. Cada cláusula é uma disjunção (OU lógico) de um conjunto de literais. Um literal é uma variável ou a negação de uma variável. O DIMACS CNF usa números inteiros positivos para representar variáveis e sua negação para representar a variável negada correspondente.

DIMACS CNF é um formato textual. Qualquer linha que comece com o caractere 'c' é considerada um comentário.

## Estrutura do CNF

Um arquivo DIMACS começa com uma linha de cabeçalho no formato `p cnf -variables- -clauses-`. Onde `-variables-` e `-clauses-` são substituídos por números decimais que indicam o número de variáveis e cláusulas na fórmula.

Após a linha de cabeçalho, seguem-se as cláusulas da fórmula. As cláusulas são codificadas como uma sequência de números decimais separados por espaços e quebras de linha. Para cada cláusula, os literais contidos são listados seguidos por um ponto 0. Normalmente, cada cláusula é listada em uma linha separada, com espaços entre cada um dos literais e o zero final.

### Exemplo de CNF

Por exemplo, a fórmula `(x ∨ y ∨ ¬z) ∧ (¬y ∨ z)` poderia ser codificada da seguinte forma:

```
p cnf 3 2
1 2 -3 0
-2 3 0
```

### Por que usar o zero ao final da cláusula

* **Tamanho variável das cláusulas:** O 0 no final da escrita de cada cláusula tem como finalidade representar um terminador da cláusula, uma vez que, como a cláusula pode ter tamanho indefinido de literais, o solver precisa de um sinal claro para saber onde a cláusula termina e a próxima começa.

* **Simplificação do Parser (Leitor de código):** Além disso, para os programas que processam o arquivo (parsers), ler números sequencialmente até encontrar um 0 é computacionalmente muito simples e rápido. O 0 age exatamente como o ponto final (.) em uma frase jurídica ou o ponto e vírgula (;) em linguagens como C e Java.

* **Otimização de espaço:** Como o 0 nunca pode ser usado para representar uma variável (as variáveis em DIMACS começam obrigatoriamente em 1), ele se torna o caractere perfeito e de "custo zero" para marcar o fim daquela instrução.

## Estrutura do código

### 1 - Structs e enum

* **valor**: É o tipo que representa o estado de uma variável: verdadeiro, falso ou ainda não atribuída. O terceiro estado existe porque a interpretação é parcial: durante a busca, parte das variáveis ainda não tem valor, e o algoritmo precisa distinguir “ainda não atribuída” de “falsa”.

* **clausula**: representa um OR (ou lógico) de literais. Cada literal é um int com sinal: o valor absoluto é a variável e o sinal indica a negação (-2 é ¬x2). O campo `num_literais` diz quantos elementos o vetor tem, e `literais` aponta para esse vetor. É usado o ponteiro e não vetor fixo porque cada cláusula pode ter um tamanho distinto, só conhecido na leitura do arquivo.

* **Formula**: representa o CNF inteiro, um AND de cláusulas. Guarda cláusulas, um ponteiro para o vetor de cláusulas, e `num_clausulas`, que diz quantas cláusulas tem na fórmula. Guarda também `num_variaveis`, necessário para saber o tamanho da interpretação.

* **interpretação**: Guarda o valor atual de cada variável. `valores` é um vetor em que a posição v guarda o valor da variável xv. Para isso valer, ele precisa de `num_variaveis + 1` posições, com a posição 0 sem uso. Ela é uma struct separada da fórmula porque muda a cada passo da busca, enquanto a fórmula permanece fixa, como no slide, onde sat(F, I) recebe os dois separadamente.

* **node**: representa um nó da árvore de busca que o SAT constrói enquanto explora as possibilidades. Cada chamada da função SAT corresponde a um nó. Tem quatro campos:

  * **variavel**: qual variável xi foi escolhida para ramificar naquele nó. Fica 0 enquanto nenhuma variável foi escolhida, e é o valor que permanece nas folhas, já que elas não ramificam.

  * **estado**: resultado da avaliação da fórmula com a interpretação parcial naquele momento. Reaproveita o enum valor, com três significados:

    * `FALSO` -> folha podada (a fórmula já é falsa, não adianta continuar)

    * `VERDADEIRO` -> folha de sucesso (a fórmula já é verdadeira)

    * `INDEFINIDO` -> nó interno, ainda não dá para saber, então ramifica

    * O nó começa como INDEFINIDO ao ser criado.

  * **left**: ponteiro para o filho do ramo TRUE (variável escolhida = verdadeiro)

  * **right**: ponteiro para o filho do ramo FALSE (variável escolhida = falso)

  * Um filho NULL significa "esse nó não existe". Numa folha os dois são NULL. Num nó interno, `right == NULL` significa que o ramo false nunca foi visitado, porque o ramo true já achou a solução.

#### Possíveis dúvidas

* **enum x struct**: O struct agrupa diferentes tipos de dados em uma única estrutura. Já o enum define um conjunto fixo de constantes nomeadas para representar estados ou opções.

* **Por que a interpretação está fora da fórmula**: A interpretação muda a cada passo da busca, enquanto a fórmula continua fixa. Caso a interpretação estivesse junto da fórmula, a cada chamada você teria que copiar a fórmula novamente.

* **Por que o node usa "struct node \*" e não só "node \*"**: Dentro da definição da struct o nome node (criado pelo typedef) ainda não existe, só existe depois do ponto e vírgula final. Por isso os ponteiros left e right precisam usar o nome longo "struct node". É o mesmo padrão de qualquer lista encadeada ou árvore em C.

* **Por que a interpretação não fica dentro do node**: A interpretação é um único vetor que vai sendo alterado e desfeito durante o backtracking, então não precisa de uma cópia por nó. A árvore só serve para registrar o caminho percorrido (qual variável foi escolhida e o estado de cada nó), para poder imprimir a busca depois.

* **Por que o estado do node usa o enum valor**: Os três resultados possíveis de avaliar a fórmula (verdadeira, falsa, indefinida) são exatamente os três valores que o enum já tem, então não precisa criar um tipo novo.

### 2 - Funções para avaliação da interpretação parcial

* **avaliar literal**

  * recebe o literal e a interpretação parcial;

  * atribui o literal a uma variável que recebe o valor absoluto do literal se ele for < 0.

  * verifica o valor desse literal na interpretação parcial e retorna conforme ele.

  **--- Casos**

  1. **variável é indefinida (== INDEFINIDO)**: retorno INDEFINIDO. No enum, FALSO vale 0, VERDADEIRO vale 1 e INDEFINIDO vale 2, então não se compara com 0 (0 é o FALSO).

  2. **literal > 0 (positivo)**: aqui o valor do literal vai ser igual ao valor da variável, então retorna a própria variável.

  3. **literal < 0**: no começo do código, atribuímos o literal à variável e depois verificamos se ela é menor que 0, caso afirmativo, multiplicamos por -1 a fim de tornar a variável positiva já que a interpretação parcial só possui índices positivos. Na verificação, como a variável possui valor contrário ao do literal, se a variável for FALSO significa que o literal é VERDADEIRO, e vice-versa.

* **avaliar clausula**

  * uma cláusula é uma disjunção (OU lógico) de literais.

  * A ideia para fazer a função é verificar literal a literal em busca de um indefinido ou um verdadeiro. Como se trata de um OU lógico, um único verdadeiro torna a cláusula verdade, assim se achamos ele, retornamos VERDADEIRO; caso ache um indefinido, não é possível retornar direto, pois ainda pode aparecer um verdadeiro, então guardamos a informação de que existe um indefinido. Só é retornado FALSO, se não tiver indefinido ou verdadeiro.

* **avaliar formula**

  * uma fórmula é uma conjunção (E lógico) de cláusulas.

  * A ideia para fazer a função é verificar cláusula a cláusula em busca de um indefinido ou um falso. Como se trata de um E lógico, um único falso torna a fórmula falsa, assim se achamos ele, retornamos FALSO; caso ache um indefinido, não é possível retornar direto, pois ainda pode aparecer um falso, então guardamos a informação de que existe um indefinido. Só é retornado VERDADEIRO, se não tiver indefinido ou falso.

### 3 - Função SAT()

* dividida em 2 partes: selecionar a variável xi para atribuir à interpretação e a própria função SAT.

* **Selecionar variável**

  * recebe a interpretação

  * faz um laço for do 1 até o número de variáveis + 1

    * começa do 1 porque `valores[0]` não é usado para guardar um valor, servindo como um sinal de ambiguidade e possível erro no código.

  * se o valor que está guardado na posição i do vetor de valores for indefinido, que é o que estamos procurando, retornamos o índice i.

  * caso não ache, retorna 0 (não deve acontecer!)

* **SAT**

  * recebe a fórmula, a interpretação e o `no_atual` (o nó da árvore que corresponde a esta chamada; quem chama já criou ele com `criar_no`).

  * cria-se uma variável e atribui a ela o valor da fórmula aplicada na interpretação parcial.

  * grava esse resultado em `no_atual->estado`, antes de decidir qualquer coisa. É isso que permite ao `imprimir_arvore` saber depois se o nó é folha podada, folha de sucesso ou nó interno.

    * **CASOS BASES**

    * caso a interpretação parcial faça a fórmula ser verdadeira, retorna true.

    * caso a interpretação parcial faça a fórmula ser falsa, retorna falso (poda da árvore).

      * poda da árvore: podar a árvore é cortar um ramo da árvore sem explorá-lo, por já ter certeza de que aquele ramo está errado. Isso garante uma economia na quantidade de nós que serão visitados.

  * se os dois ifs não rodarem, escolho a variável pela função auxiliar e guardo ela em `no_atual->variavel` (é o xi que esse nó ramifica).

  * **RAMO TRUE**

    * atribuo a essa variável VERDADEIRO, crio o filho esquerdo com `criar_no` (`no_atual->left`) e chamo recursivamente SAT(fórmula, interpretação + xi escolhido, `no_atual->left`).

  * **RAMO FALSE**: só ocorre se o ramo true falhou

    * sobrescrevo o valor da variável como FALSO, crio o filho direito com `criar_no` (`no_atual->right`) e chamo recursivamente SAT(fórmula, interpretação + xi escolhido, `no_atual->right`).

    * como o filho direito só é criado aqui, se o ramo true achar a solução o right fica NULL: é isso que o `imprimir_arvore` mostra como "F: (não visitado)".

  * Caso os dois ramos falharem, a variável é sobrescrita como INDEFINIDA e a função devolve false, deixando o vetor como recebeu. Os nós criados continuam na árvore (não são apagados no backtracking), porque a árvore serve justamente para mostrar tudo o que foi explorado; a interpretação é que é desfeita.

### 4 - Funções para criar a interpretação, criar o nó e liberação de memória

* **criar interpretação**

  * recebe um inteiro n que significa o número de variáveis.

  * cria a variável interpretação e aloca-se memória para ela.

  * atribui n ao número de variáveis em interpretação.

  * aloca-se memória para o vetor de valores `((n + 1) * sizeof(valor))`: o +1 é por causa da posição 0, que fica sem uso, para que a variável xv fique na posição v.

  * em um for de 1 até n, é pré-setado todo o vetor de valores como INDEFINIDO (a posição 0 não é mexida).

  * retorna o endereço de memória da interpretação.

* **criar nó**

  * não recebe nada, devolve o endereço de um node novo.

  * aloca memória para um node.

  * `variavel = 0` (nenhuma variável escolhida ainda).

  * `estado = INDEFINIDO` (ainda não foi avaliado).

  * `left = NULL` e `right = NULL` (ainda não tem filhos).

  * retorna o endereço do nó.

  * é chamada na main, para criar a raiz, e dentro do SAT, para criar o filho esquerdo e o direito só quando cada ramo vai ser explorado.

* **free interpretação**

  * recebe uma interpretação.

  * se a interpretação for nula, "faço nada".

  * caso contrário, libero o vetor de valores ligado à interpretação e, após isso, libero a interpretação.

* **free formula**

  * recebe uma fórmula.

  * se a fórmula for nula, "faço nada".

  * caso contrário, faz um for de 0 até o número de cláusulas, liberando os literais de cada cláusula que está no vetor de cláusulas ligadas à fórmula.

    * após, libera o vetor de cláusulas ligadas à fórmula e a própria fórmula.

* **free arvore**

  * recebe a raiz de uma (sub)árvore.

  * se a raiz for nula, "faço nada" (caso base da recursão; cobre folhas e o ramo false não visitado).

  * caso contrário, chama `free_arvore` para o filho esquerdo, depois para o direito, e só então libera o próprio nó.

    * a ordem importa (pós-ordem): se liberasse o nó antes, perderia os ponteiros left e right e não teria como chegar nos filhos.

### 5 - Função para ler no formato DIMACS

* recebe o nome do arquivo (`char *`).

* devolve a fórmula em forma de ponteiro (`formula *`), ou NULL se algo deu errado (mensagem do motivo foi impressa).

* **ideia central**: lê o arquivo em três etapas, nessa ordem: (1) pula os comentários, (2) lê o cabeçalho `p cnf V C`, (3) lê exatamente C cláusulas, cada uma terminando em 0. Diferente de uma leitura palavra por palavra, aqui o fscanf já lê o número pronto (`%d`) e vai guardando cada literal direto na cláusula, sem vetor temporário e sem flags de estado.

* **1: Abrir o arquivo FILE** descreve a variável; `fopen` abre o arquivo e associa ele a um fluxo, permitindo leitura no nosso caso "r"; se o arq for NULL (não abrir) imprime o erro e retorna NULL.

* **2: pular os comentários**

  * `char letra`: guarda o primeiro caractere de cada linha lida.

  * laço: `while(fscanf(arq, " %c", &letra) == 1 && letra == 'c')`

    * o espaço antes do `%c` manda o fscanf ignorar espaços e quebras de linha, então o que é lido é sempre o primeiro caractere que não é branco.

    * o fscanf devolve quantos itens leu; `== 1` significa que leu a letra com sucesso (se o arquivo acabar devolve EOF e o laço para).

    * se a letra for 'c', a linha é comentário e o corpo do laço roda: `fscanf(arq, "%*[^\n]")` lê e descarta tudo até o fim da linha. O `[^\n]` significa "qualquer caractere que não seja quebra de linha" e o `*` diz para ler, mas não guardar em nenhuma variável.

  * o laço termina quando a primeira letra de uma linha não é 'c'. Essa letra fica guardada em `letra` e deve ser o 'p' do cabeçalho.

* **3: ler o cabeçalho**

  * `int v, c`: número de variáveis e número de cláusulas.

  * condição de erro: `letra != 'p' || fscanf(arq, " cnf %d %d", &v, &c) != 2 || v < 0 || c < 0`

    * o `||` é avaliado da esquerda para a direita e para no primeiro verdadeiro, então se `letra` não for 'p' o fscanf nem chega a ser chamado.

    * no formato `" cnf %d %d"`, o espaço inicial ignora brancos, "cnf" precisa aparecer exatamente igual, e os dois `%d` leem V e C. O 'p' já foi consumido no passo 2, por isso não aparece no formato.

    * o fscanf devolve 2 se leu os dois números; qualquer outra coisa significa cabeçalho incompleto ou fora do formato.

    * `v` ou `c` negativos não fazem sentido, então também são erro.

  * se der erro: imprime a mensagem, fecha o arquivo e retorna NULL.

  * esse mesmo teste cobre o arquivo vazio ou só com comentários, porque nesses casos `letra` não vai ser 'p'.

* **4: preparar a formula**

  * `form`: fórmula que será devolvida; aloca espaço, guarda `num_variaveis = v` e `num_clausulas = c`.

  * `clausulas` é alocado com `calloc(c, sizeof(clausula))`. O calloc, além de alocar, zera a memória, então toda cláusula começa com `num_literais = 0` e `literais = NULL`.

    * Isso é importante por dois motivos: a cláusula já nasce pronta para receber o primeiro realloc, e se der erro no meio da leitura o `free_formula` consegue liberar tudo com segurança (`free(NULL)` não faz nada), inclusive as cláusulas que nem chegaram a ser lidas.

* **5: laço das cláusulas**: for i de 0 até c, lê exatamente c cláusulas.

  * `cl = &form->clausulas[i]`: ponteiro para a cláusula que está sendo preenchida, só para não repetir `form->clausulas[i]` toda hora.

  * `lit = -1`: guarda o último número lido. O valor inicial não importa, só precisa ser diferente de 0 para o laço de dentro poder começar.

  * laço interno: `while(fscanf(arq, "%d", &lit) == 1 && lit != 0 && abs(lit) <= v)`

    * lê um número inteiro por vez (o `%d` sem espaço também ignora brancos e quebras de linha, por isso a cláusula pode estar espalhada por várias linhas).

    * só entra no corpo se: o fscanf leu um número, esse número não é 0 (0 fecha a cláusula) e o literal é válido, ou seja, `|lit| <= v` (abs vem do `stdlib.h`).

    * corpo: realloc aumenta o vetor de literais em 1 posição `((num_literais + 1) * sizeof(int))`, grava o literal na última posição e incrementa `num_literais`.

    * como o realloc começa com `literais = NULL`, o primeiro realloc se comporta como um malloc.

    * o vetor cresce exatamente do tamanho que a cláusula tem, por isso não precisa de buffer temporário nem de capacidade extra.

  * verificação depois do laço: se `lit != 0`, o laço parou sem ler o 0 que fecha a cláusula. Isso acontece quando:

    * o arquivo acabou antes da cláusula terminar (faltou o 0 ou faltam cláusulas).

    * apareceu algo que não é número.

    * o literal estava fora do intervalo (`|lit| > v`).

    * nesses casos imprime erro, fecha o arquivo, chama `free_formula(form)` e retorna NULL.

  * se `lit == 0`, a cláusula foi fechada corretamente e o for passa para a próxima. Uma cláusula só com o 0 fica com `num_literais = 0`, que é uma cláusula vazia, e o `avaliar_clausula` já a trata como FALSO.

* **6: fechamento e retorno**

  * `fclose(arq)` fecha o arquivo e retorna `form`.

  * como o laço só lê c cláusulas, qualquer texto que sobrar depois da última é simplesmente ignorado.

#### Limitações dessa versão

* comentários só são aceitos antes do cabeçalho; um 'c' no meio das cláusulas faz o `%d` falhar e retorna erro.

* não existe mais o erro específico de "cláusulas demais ou de menos": se o arquivo tiver menos que c cláusulas dá erro, se tiver mais, as que sobram são ignoradas.

### 6 - Função imprimir árvore

* recebe o nó que vai ser impresso; a profundidade (altura); e um char representando a aresta observada (T ou F).

* `if(raiz == NULL)`: este é o caso base, todo nó folha tem dois filhos com valor NULL e um nó interno cujo ramo false nunca foi visitado tem `right == NULL`, quando chegamos em um desses casos não é preciso mais imprimir nada.

* `for(i = 0; i < profundidade)`: Indentação, ajuda a deixar a impressão da árvore mais fácil de visualizar, a raiz (0) não recebe espaço.

* `if(aresta != ' ')`: Rótulo da aresta, se a aresta não for T ou F não imprime nada, a raiz não imprime nada aqui.

* conteúdo do nó, escolhido pelo estado que a função SAT gravou:

  * **FALSO**: folha podada (X do slide) -> imprime [X] fórmula falsa (poda).

  * **VERDADEIRO**: folha de sucesso -> [OK] fórmula verdadeira.

  * **INDEFINIDO**: nó interno que ramificou -> x seguido do número da variável.

* Recursão em pré-ordem: chama a função pelo nó da esquerda (que guarda o valor true), aumenta a profundidade em + 1; depois faz a mesma coisa com o nó da direita (que guarda o valor false).

* Ramo da direita (false) não visitado: depois de imprimir os filhos, se o nó interno é INDEFINIDO e o right é NULL, imprime "F: (não visitado)" na posição em que o filho estaria. Isso ocorre quando o ramo true já achou a solução e o false não foi criado. O teste de estado evita imprimir isso nas folhas, que também têm o `right == NULL`.

### 7 - A main

* A main é o ponto de entrada. Os dois parâmetros `argc` e `argv` referem-se ao que é digitado na linha de comando.

  * **argc (argument count)**: é quantas palavras havia na linha de comando, contando o nome do programa.

  * **argv (argument vector)**: é o vetor com essas palavras, cada uma como texto.

  * O tipo `char *argv[]` se lê como "vetor de ponteiros para char". Cada string aponta para uma string terminada em `\0`.

    * `argv[0]` sempre é o nome do programa, então argc nunca é menor que 1. Os argumentos do usuário começam em `argv[1]`.

    * `argv[argc]` é sempre NULL.

  * **validar o argumento**

    * sem pelo menos um argumento (arquivo), não tem o que fazer e o programa imprime o modo de uso e termina.

    * return 1, se entrar na condicional, significa, por convenção, que houve algum erro.

  * **ler a fórmula**

    * `argv[1]` é o nome do arquivo digitalizado. O `ler_dimacs` retorna a fórmula pronta ou NULL, se der algum erro.

  * **preparar a busca**

    * a interpretação é criada com o número de variáveis lido no arquivo.

    * a raiz da árvore é criada.

  * resolver usando a função SAT e imprimir a árvore.