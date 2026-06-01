# Conceito do Jogo — Game Jam “2 Mundos”

O projeto surgiu dentro de uma game jam pequena com cerca de sete equipes, muitas delas formadas por iniciantes. Como diferencial técnico e criativo, o jogo está sendo desenvolvido inteiramente em C utilizando uma engine própria chamada “PetGraphycs”, criada anteriormente como experimento pessoal. A engine possui um estilo visual extremamente minimalista e de baixa resolução, no qual sprites muito pequenos (como 16x16) ocupam grande espaço na tela, produzindo uma estética retrô e simplificada.

Inicialmente, a limitação gráfica parecia um problema, especialmente para um jogo de plataforma 2D tradicional. Porém, ao invés de lutar contra essa limitação, a proposta passou a incorporá-la diretamente na identidade do jogo. O resultado foi um conceito que conecta mecânica, narrativa, estética e implementação técnica em torno do tema da jam: “2 mundos”.

A ideia central é que existam dois níveis de realidade coexistindo:

1. O mundo “normal”, percebido pelo jogador como um jogo de plataforma convencional.
2. O “mundo do código”, uma camada inferior da realidade onde o funcionamento interno do jogo pode ser visto e manipulado.

No mundo normal, o jogador:

* anda,
* pula,
* explora cenários,
* resolve puzzles ambientais.

Já no mundo do código, o jogador literalmente enxerga a implementação lógica da realidade:

* variáveis,
* estados,
* colisões,
* enums,
* comparadores,
* flags,
* estruturas internas do jogo.

A proposta não é representar um “mundo digital” genérico, mas sim transformar o próprio backend da realidade em mecânica jogável.

O aspecto mais importante do conceito é a transição entre dois modos de interação:

## 1. Modo Responsivo (Gameplay tradicional)

O jogo funciona como um plataforma 2D comum:

* animações contínuas,
* movimentação em tempo real,
* física,
* exploração.

O programa continua executando constantemente enquanto aguarda inputs do jogador, como qualquer jogo normal.

## 2. Modo Eventivo (Terminal / edição da realidade)

Em pontos específicos do mapa, o jogador acessa “zonas de execução”, nas quais o jogo abandona temporariamente o comportamento contínuo e assume uma interface semelhante a um terminal.

Nesse estado:

* o jogo pausa,
* o jogador recebe acesso ao “código” do cenário,
* e pode editar partes limitadas da lógica da realidade.

A inspiração vem diretamente de programas de terminal escritos em C, nos quais a execução aguarda explicitamente uma entrada do usuário através de algo semelhante a um `scanf`.

A ideia é que o jogador não esteja apenas jogando dentro do mundo, mas invadindo a camada lógica que controla sua existência.

## Filosofia de Design

Um dos maiores riscos identificados durante o planejamento foi transformar o projeto em um “mini compilador” ou “mini interpretador de C”, algo inviável dentro do escopo de uma game jam.

Por isso, o objetivo não é permitir programação livre, mas sim criar a ILUSÃO de programação.

Ao invés de interpretar código real, o sistema funciona através da manipulação limitada de tokens específicos:

* números,
* operadores,
* booleanos,
* enums,
* estados.

O jogador poderá:

* mover o cursor por um editor de texto simples,
* apagar tokens inteiros,
* substituir valores permitidos,
* alterar operadores e estados pré-definidos.

Isso mantém:

* a sensação de hacking,
* a estética de programação,
* e os puzzles baseados em lógica computacional,
  sem exigir um parser complexo.

## Exemplos de Puzzle

### Alterar estados booleanos

```c
bridge.enabled = false;
```

O jogador altera:

```c
false -> true
```

E a ponte aparece instantaneamente no cenário.

### Alterar operadores

```c
if(player.keys < 3)
```

O jogador substitui:

```c
< -> >=
```

Mudando a lógica que controla uma porta.

### Manipular enums

```c
player.gravity = 0;
```

O cenário fornece pistas de que:

```c
0 = DOWN
1 = UP
```

Então o jogador altera:

```c
0 -> 1
```

Invertendo a gravidade.

### Alterar estados de IA

```c
enemy.state = 0;
```

Onde:

```c
0 = AGGRESSIVE
1 = SLEEP
2 = IDLE
```

O jogador modifica o comportamento do inimigo diretamente.

## Relação entre estética e narrativa

A baixa resolução dos gráficos deixa de ser uma limitação e passa a fazer parte do universo do jogo.

No mundo do código:

* simplificações visuais,
* grids,
* hitboxes,
* glitches,
* wireframes,
* e representações abstratas

passam a existir de forma diegética, como se o jogador estivesse observando a implementação real do universo.

Dessa forma:

* a engine em C,
* a estética minimalista,
* as limitações técnicas,
* e a temática de “2 mundos”

deixam de ser elementos separados e passam a reforçar uns aos outros.

## Objetivo Final

O foco do projeto não é competir por escala ou quantidade de conteúdo, mas por identidade.

A proposta é criar um jogo pequeno, coeso e memorável, no qual:

* programação vira mecânica,
* debugging vira puzzle,
* e o jogador aprende a “hackear” a realidade para avançar.

# Conceito do Jogo — Parte 2: Evolução do Sistema de “Dois Mundos”

Após a consolidação da ideia inicial — um jogo de plataforma em C no qual o jogador alterna entre um mundo normal e um “mundo do código” — o conceito evoluiu para uma abordagem mais enxuta, controlável e mecanicamente consistente, sem perder a essência temática da game jam “2 mundos”.

Em vez de tratar os dois mundos como realidades separadas ou estados completos de jogo, a nova proposta redefine essa dualidade como **camadas de leitura da mesma realidade**.

Ou seja, existe apenas um mundo em execução contínua, mas ele pode ser interpretado de diferentes formas através de modos de debug.

## Dois Mundos como Camadas de Interpretação

O “mundo do código” deixa de ser um espaço separado e passa a ser um conjunto de ferramentas de observação e manipulação da realidade. O jogador não troca de mundo, mas sim de perspectiva sobre o mesmo sistema.

Essa abordagem resolve dois problemas importantes do design original:

* reduz a complexidade de implementação (evitando dois mundos paralelos completos),
* e fortalece a identidade do jogo como uma simulação computacional coerente.

Agora, o tema “2 mundos” passa a ser interpretado como:

> **realidade perceptível vs realidade estrutural**

---

## Sistema de Debug Modes

O jogo passa a contar com diferentes modos de debug, ativados em pontos específicos ou como ferramentas de puzzle. Cada modo revela um aspecto diferente do funcionamento interno do mundo.

### 1. Debug de Colisão

Exibe:

* hitboxes,
* áreas sólidas,
* zonas de interação,
* triggers ocultos.

Esse modo permite que o jogador compreenda por que certos caminhos estão bloqueados ou acessíveis, transformando a geometria do cenário em informação explícita.

---

### 2. Debug de Variáveis

Exibe o estado interno de entidades e objetos do mundo:

* estados de portas,
* gravidade do jogador,
* flags de interação,
* estados de inimigos.

Exemplo:

```text
door.locked = true
player.gravity = DOWN
enemy.state = AGGRESSIVE
```

Esse modo transforma o cenário em um sistema legível, permitindo que o jogador entenda a lógica por trás dos comportamentos observados.

---

### 3. Debug de Estrutura / Memória

Exibe informações sobre a organização interna dos elementos:

* se um objeto é estático ou dinâmico,
* se é um array ou entidade única,
* origem lógica de determinados elementos,
* relações entre estruturas.

Exemplo:

```text
tile[12] (heap allocated)
door_ptr -> 0x00FF32A1
```

Esse nível adiciona uma camada mais abstrata de leitura do mundo, aproximando o jogador da ideia de “ver a implementação da realidade”.

---

### 4. Debug de Fluxo Lógico

Exibe relações entre elementos interativos:

* conexões entre botões e portas,
* dependências de eventos,
* fluxos de ativação,
* caminhos de IA.

O cenário deixa de ser apenas espacial e passa a ser um grafo de interações visível.

---

## Design Baseado em Camadas

Com essa reformulação, o jogo deixa de exigir a manutenção de dois estados completos de mundo e passa a operar sobre um único sistema com múltiplas camadas informacionais.

Isso traz vantagens importantes:

* maior controle sobre o design dos puzzles,
* implementação mais simples e previsível,
* redução de bugs sistêmicos,
* e melhor clareza para o jogador.

---

## Estrutura de Puzzle

Os puzzles passam a ser construídos em torno de três etapas fundamentais:

1. **Observação do mundo normal**
   O jogador identifica um problema no cenário.

2. **Uso do modo de debug adequado**
   O jogador seleciona a camada de informação que revela a causa do problema.

3. **Intervenção limitada via terminal/editor**
   Em pontos específicos, o jogador pode alterar valores restritos da lógica do sistema (como enums, booleanos, operadores e índices), gerando mudanças imediatas no mundo.

Essa estrutura mantém o elemento de “hacking da realidade”, mas dentro de um escopo controlado e implementável.

---

## Filosofia Atualizada do Projeto

O jogo evolui de uma ideia de “alternância entre mundos” para um conceito mais refinado:

> O jogador não muda o mundo — ele muda a forma como o mundo é interpretado e depurado.

A realidade do jogo não é duplicada, mas sim exposta em diferentes níveis de abstração, como se o jogador estivesse navegando por ferramentas internas de uma engine viva.

---

## Resultado da Evolução

Essa abordagem preserva os objetivos originais:

* identidade forte baseada em C e computação,
* puzzles lógicos com leitura de sistema,
* estética minimalista coerente com limitações técnicas,
* e integração entre narrativa e mecânica.

Ao mesmo tempo, torna o projeto:

* mais leve de implementar,
* mais fácil de balancear,
* e mais consistente como experiência de jogo.

O tema “2 mundos” permanece, mas agora de forma mais sofisticada e interpretável: dois mundos não como lugares distintos, mas como **duas formas de ver e interagir com a mesma realidade computacional**.
