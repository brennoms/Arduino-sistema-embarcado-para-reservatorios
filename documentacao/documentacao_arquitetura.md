---
id: documentacao_projeto
title: Documentação da Arquitetura — Sistema Programável de Três Bombas
---

# Documentação da Arquitetura — Sistema Programável de Três Bombas

## 1. Visão geral

Este projeto implementa, sobre um **Arduino Mega 2560**, uma arquitetura de computador simulada para monitoramento e controle de três reservatórios.

A aplicação física utiliza três potenciômetros como entradas analógicas, representando os níveis dos reservatórios, e LEDs para representar as bombas e os indicadores de manutenção. O programa que controla o sistema é escrito em uma **ISA própria**, em Assembly, carregado pelo monitor serial e convertido pelo montador para palavras binárias armazenadas em uma memória de programa simulada.

A execução é realizada segundo o modelo clássico de **programa armazenado**, seguindo o ciclo:

```text
Assembly
   ↓
ASSEMBLER
   ↓
Palavra binária de 18 bits
   ↓
MEM_PROG
   ↓
Busca
   ↓
IR
   ↓
DECODER
   ↓
UC
   ↓
ULA / E-S / PC / ACC / FLAGS
```

A arquitetura simulada é independente da arquitetura interna do ATmega2560. O microcontrolador fornece os recursos físicos necessários para executar o simulador, enquanto UC, ULA, memória, registradores, Decoder e ISA são estruturas implementadas em software.

---

## 2. Organização da arquitetura

Os principais módulos da arquitetura são:

```text
                         ┌───────────────────────┐
                         │          UC           │
                         │ Busca / Decodificação │
                         │       / Execução      │
                         └───────────┬───────────┘
                                     │
                                  DECODER
                                     │
              ┌──────────────────────┼──────────────────────┐
              │                      │                      │
             ULA                  MEMÓRIA                  E/S
              │                      │                      │
        ACC / FLAGS             MAR / MBR             Entradas / Saídas
                                     │
                                    IR
                                     │
                                    PC
```

### 2.1 Unidade de Controle (UC)

A UC é responsável por coordenar o ciclo de instrução. Ela determina a sequência de operações dos demais módulos.

O ciclo interno é dividido em três fases principais:

1. **BUSCA** — utiliza o `PC` para localizar a próxima instrução, coloca o endereço no `MAR`, solicita leitura à memória e recebe a palavra no `MBR`.
2. **DECODIFICAÇÃO** — copia a palavra do `MBR` para o `IR` e utiliza o `DECODER` para extrair opcode e operandos.
3. **EXECUÇÃO** — chama o módulo necessário para realizar a operação e atualiza os registradores e o `PC`.

A UC, portanto, é o elemento que faz a comunicação entre os módulos por meio dos registradores.

### 2.2 Decoder

O Decoder recebe a palavra armazenada em `IR` e separa seus campos:

```text
IR[17..14] → OPCODE
IR[13..7]  → OPERANDO 1
IR[6..0]   → OPERANDO 2
```

O opcode é convertido em uma instrução da ISA. O código `1111` é reservado para combinação inválida.

### 2.3 Unidade Lógica e Aritmética (ULA)

A ULA realiza a operação aritmética utilizada por `CMP`:

```text
NIVEL[x] - n
```

O resultado é colocado em `ACC` e utilizado para atualizar os três indicadores de comparação:

```text
FLAG_L → resultado < 0
FLAG_Z → resultado = 0
FLAG_G → resultado > 0
```

Exemplo:

```text
NIVEL = 20
limite = 30

ULA:
20 - 30 = -10

ACC    = -10
FLAG_L = 1
FLAG_Z = 0
FLAG_G = 0
```

### 2.4 Memória

`MEM_PROG` é a memória de programa da arquitetura simulada. Cada posição armazena uma instrução completa, sem necessidade de dividir uma instrução entre posições diferentes.

A implementação também possui `MEM_OCUPADA`, que informa quais posições realmente foram carregadas. Dessa forma, uma palavra composta apenas por zeros não é confundida com uma posição vazia.

### 2.5 Entrada e saída

As funções de E/S conectam a arquitetura simulada ao hardware do Arduino:

- leitura dos três potenciômetros;
- acionamento das três bombas representadas por LEDs;
- LEDs de manutenção preventiva;
- buzzer para alarmes corretivos;
- display de sete segmentos.

---

# 3. Definição da palavra da ISA

## 3.1 Largura da palavra

A arquitetura utiliza uma **palavra de 18 bits**.

A palavra é dividida em três campos de tamanho fixo:

```text
17                         14 13                       7 6             0
┌────────────────────────────┬───────────────────────────┬───────────────┐
│          OPCODE            │       OPERANDO 1         │  OPERANDO 2   │
│           4 bits           │          7 bits          │    7 bits     │
└────────────────────────────┴───────────────────────────┴───────────────┘
```

Logo:

```text
4 + 7 + 7 = 18 bits
```

A escolha de 18 bits é justificada pelos requisitos da ISA e pelo maior formato de instrução utilizado pelo projeto: `CMP x,n`, que necessita de dois operandos.

### 3.2 Justificativa dos 4 bits de opcode

A ISA possui 15 instruções:

```text
READ
ON
OFF
ALARM
LED
INFO
SILENCE
LEDOFF
CMP
JMP
JL
JE
JG
WAIT
HALT
```

Para representar 15 códigos distintos, são necessários 4 bits:

```text
2^3 = 8   → insuficiente
2^4 = 16  → suficiente
```

Assim, quatro bits permitem representar os 15 opcodes utilizados e ainda deixam uma combinação disponível:

```text
1111 → opcode inválido/reservado
```

### 3.3 Justificativa dos campos de 7 bits

Cada operando possui 7 bits porque o projeto utiliza endereços de 7 bits e a memória possui 128 posições.

Com 7 bits, temos:

```text
2^7 = 128 valores
```

Portanto, é possível representar exatamente todos os endereços:

```text
0000000 → 0x00
1111111 → 0x7F
```

Além dos endereços, o campo também é suficiente para representar:

- bombas `1`, `2` e `3`;
- percentuais de `0` a `100`;
- tempos de espera dentro da faixa representável pela ISA.

### 3.4 Por que não utilizar uma palavra maior?

Uma palavra de 18 bits é suficiente para o conjunto de instruções definido. Aumentá-la para 24 ou 32 bits não acrescentaria capacidade funcional necessária à ISA atual, pois:

- o opcode já cabe em 4 bits;
- um endereço já cabe em 7 bits;
- `CMP` utiliza dois operandos de 7 bits;
- os campos restantes não são necessários para as instruções atuais.

Portanto, 18 bits representam a menor largura natural para o formato escolhido:

```text
OPCODE + OPERANDO 1 + OPERANDO 2
   4   +      7     +      7
   = 18 bits
```

Os campos não utilizados por determinadas instruções são preenchidos com zero. Isso mantém todas as instruções com o mesmo tamanho e simplifica a busca e a decodificação.

---

# 4. Tamanho e capacidade da memória

## 4.1 Quantidade de posições

A memória possui **128 posições**.

Como cada endereço é representado por 7 bits:

```text
2^7 = 128 posições
```

Os endereços válidos são:

```text
0x00 até 0x7F
```

A numeração decimal correspondente é:

```text
0 até 127
```

Cada posição armazena uma instrução completa de 18 bits.

## 4.2 Capacidade lógica da memória

A capacidade lógica é calculada por:

```text
quantidade de posições × tamanho da palavra

128 × 18 = 2304 bits
```

Convertendo para bytes:

```text
2304 / 8 = 288 bytes
```

Portanto, a memória de programa possui uma capacidade arquitetural de:

> **2304 bits = 288 bytes de informação útil.**

Outra forma de expressar a capacidade é:

```text
128 palavras × 18 bits por palavra
```

## 4.3 Capacidade de endereçamento

Como cada endereço aponta diretamente para uma palavra completa, uma posição de memória corresponde a uma instrução inteira:

```text
0x00 → palavra 0
0x01 → palavra 1
0x02 → palavra 2
...
0x7F → palavra 127
```

Isso significa que a arquitetura não precisa utilizar posições consecutivas para montar uma instrução de `18 bits`.

## 4.4 Representação da memória no código

No sketch, a memória é implementada como:

```c
uint32_t MEM_PROG[TAM_MEMORIA];
```

com:

```c
#define TAM_MEMORIA 128
```

Embora cada posição utilize um `uint32_t` por conveniência de armazenamento e manipulação no Arduino, apenas os **18 bits menos significativos** são considerados parte da palavra da arquitetura:

```text
32 bits físicos de armazenamento
┌────────────────────────────────┐
│              14               │ 18 bits usados │
└────────────────────────────────┘
                               ↑
                         palavra da ISA
```

Assim, existe uma diferença entre:

- **capacidade lógica da memória:** `128 × 18 = 2304 bits = 288 bytes`;
- **armazenamento utilizado pelo array `MEM_PROG`:** `128 × 32 = 4096 bits = 512 bytes`.

O espaço adicional no `uint32_t` não representa memória arquitetural adicional; ele é apenas uma estratégia de implementação para armazenar e manipular facilmente uma palavra de 18 bits.

Além disso, o projeto mantém:

```c
bool MEM_OCUPADA[TAM_MEMORIA];
```

para controlar quais posições foram carregadas.

---

# 5. Registradores da arquitetura

| Registrador | Largura / tipo | Função |
|---|---:|---|
| `PC` | 7 bits efetivos | Endereço da próxima instrução |
| `MAR` | 7 bits efetivos | Endereço utilizado no acesso à memória |
| `MBR` | 18 bits efetivos | Palavra transferida entre memória e CPU |
| `IR` | 18 bits efetivos | Palavra completa da instrução atual |
| `ACC` | `int16_t` | Resultado de `READ`, `INFO` e `CMP` |
| `OPCODE_REG` | 4 bits | Opcode extraído do `IR` |
| `OP1` | 7 bits | Primeiro operando |
| `OP2` | 7 bits | Segundo operando |
| `NIVEL[3]` | 7 bits úteis | Último nível válido de cada reservatório |
| `NIVEL_VALIDO[3]` | booleano | Indica se existe leitura válida |
| `FLAG_L` | 1 bit | Resultado menor que zero |
| `FLAG_Z` | 1 bit | Resultado igual a zero |
| `FLAG_G` | 1 bit | Resultado maior que zero |
| `COMPARACAO_VALIDA` | booleano | Indica existência de comparação válida |

O `ACC` utiliza um tipo assinado porque `CMP` pode produzir valores negativos. A faixa necessária é pequena:

```text
-100 ≤ ACC ≤ +100
```

---

# 6. Formato das instruções

A ISA utiliza um formato geral de 18 bits:

```text
OPCODE | OPERANDO 1 | OPERANDO 2
  4          7            7
```

Nem toda instrução utiliza os dois operandos. Os campos não utilizados recebem zero.

| Instrução | Opcode | Operando 1 | Operando 2 |
|---|---|---|---|
| `READ x` | `0000` | bomba | `0000000` |
| `ON x` | `0001` | bomba | `0000000` |
| `OFF x` | `0010` | bomba | `0000000` |
| `ALARM x` | `0011` | bomba | `0000000` |
| `LED x` | `0100` | bomba | `0000000` |
| `INFO x` | `0101` | bomba | `0000000` |
| `SILENCE x` | `0110` | bomba | `0000000` |
| `LEDOFF x` | `0111` | bomba | `0000000` |
| `CMP x,n` | `1000` | bomba | percentual |
| `JMP a` | `1001` | endereço | `0000000` |
| `JL a` | `1010` | endereço | `0000000` |
| `JE a` | `1011` | endereço | `0000000` |
| `JG a` | `1100` | endereço | `0000000` |
| `WAIT t` | `1101` | tempo | `0000000` |
| `HALT` | `1110` | `0000000` | `0000000` |
| inválido | `1111` | — | — |

---

# 7. Tipos de operandos

## 7.1 Identificação de bomba

As instruções relacionadas às bombas utilizam:

```text
x ∈ {1, 2, 3}
```

Exemplos:

```asm
READ 1
ON 2
OFF 3
INFO 1
```

O valor é escrito em decimal no Assembly e ocupa 7 bits na palavra binária.

## 7.2 Percentual

O segundo operando de `CMP` representa um percentual:

```text
0 ≤ n ≤ 100
```

Exemplo:

```asm
CMP 1, 30
```

O primeiro campo representa a bomba e o segundo representa o limite de comparação.

## 7.3 Endereço

Os destinos dos desvios são obrigatoriamente informados em hexadecimal com prefixo `0x` e dois dígitos:

```asm
JMP 0x0A
JL 0x10
JE 0x20
JG 0x7F
```

A restrição `0x00..0x7F` corresponde diretamente às 128 posições da memória.

## 7.4 Tempo de `WAIT`

O formato da ISA reserva 7 bits para o argumento de `WAIT`. Portanto, um valor diretamente armazenável nesse campo está na faixa:

```text
0..127
```

Cada unidade representa 100 ms.

Assim, com um campo de 7 bits, o maior valor diretamente codificável é:

```text
127 × 100 ms = 12,7 s
```

### Observação de implementação

No código atual existe a constante:

```c
#define MAX_WAIT_TICKS 600
```

Porém, `OP1` possui apenas 7 bits e é armazenado em `uint8_t`. Para manter a ISA coerente com o formato de 18 bits, o limite efetivo de `WAIT` deve ser ajustado para **127** ou o formato da ISA precisa ser alterado. Para a arquitetura documentada neste arquivo, considera-se `0..127` como a faixa representável.

---

# 8. Conjunto de instruções

## `READ x`

Lê o potenciômetro associado à bomba `x`, converte a entrada analógica para um percentual entre 0% e 100%, armazena o valor em `NIVEL[x]` e em `ACC` e marca a leitura como válida.

## `ON x`

Liga a saída correspondente à bomba `x`.

## `OFF x`

Desliga a saída correspondente à bomba `x`.

## `ALARM x`

Ativa o estado de manutenção corretiva da bomba `x` e mantém o buzzer ativo enquanto existir pelo menos um alarme corretivo ativo.

## `LED x`

Liga o LED de manutenção preventiva da bomba `x`.

## `INFO x`

Realiza nova leitura do nível da bomba `x`, atualiza `NIVEL[x]` e `ACC` e apresenta no display a faixa correspondente ao percentual.

## `SILENCE x`

Desativa o alarme corretivo da bomba `x`. O buzzer somente é desligado quando não existir outro alarme corretivo ativo.

## `LEDOFF x`

Desliga o LED de manutenção preventiva da bomba `x`.

## `CMP x,n`

Compara o último nível válido da bomba `x` com o percentual `n` sem realizar nova leitura:

```text
ACC = NIVEL[x] - n
```

Os flags são definidos a partir do resultado.

`CMP` é a única instrução que altera os indicadores de comparação.

## `JMP a`

Desvio incondicional para o endereço `a`.

## `JL a`

Desvia para `a` quando `FLAG_L` estiver ativo.

## `JE a`

Desvia para `a` quando `FLAG_Z` estiver ativo.

## `JG a`

Desvia para `a` quando `FLAG_G` estiver ativo.

## `WAIT t`

Suspende temporariamente a execução durante `t × 100 ms`, sem bloquear o monitor serial.

## `HALT`

Finaliza a execução do programa e mantém o estado das saídas.

---

# 9. Montagem da instrução

O processo de montagem ocorre somente durante a fase de carga.

```text
Linha Assembly
      ↓
Remoção de comentário
      ↓
Separação dos tokens
      ↓
Identificação do mnemônico
      ↓
Validação dos operandos
      ↓
Codificação do opcode
      ↓
Codificação dos operandos
      ↓
Palavra de 18 bits
      ↓
MBR
      ↓
MEMORIA(MEM_ESCRITA)
```

O caractere `;` inicia um comentário. Tudo depois dele é descartado antes da montagem.

Exemplo:

```asm
READ 1 ; lê o reservatório 1
```

é montado da mesma maneira que:

```asm
READ 1
```

Linhas vazias e linhas compostas somente por comentários não ocupam posições da memória.

A memória só é modificada depois que a instrução inteira foi validada. Dessa forma, uma linha inválida não deixa uma instrução parcialmente gravada.

---

# 10. Ciclo de busca, decodificação e execução

A execução segue o modelo:

### 10.1 Busca

```text
PC → MAR
```

A UC coloca no `MAR` o endereço indicado pelo `PC` e solicita uma leitura:

```text
MEMORIA(MEM_LEITURA)
```

A memória retorna a palavra em:

```text
MBR
```

### 10.2 Transferência para IR

```text
MBR → IR
```

O `IR` recebe a palavra binária completa de 18 bits.

### 10.3 Decodificação

O Decoder separa:

```text
IR[17..14] → opcode
IR[13..7]  → OP1
IR[6..0]   → OP2
```

### 10.4 Execução

A UC utiliza o opcode para selecionar a operação correspondente.

Exemplo para `CMP`:

```text
IR
 ↓
DECODER
 ↓
OP1 = bomba
OP2 = limite
 ↓
UC
 ↓
NIVEL[OP1]
 ↓
ULA
 ↓
ACC + FLAGS
```

### 10.5 Atualização do PC

Após a execução, o `PC` é incrementado para a próxima posição quando a instrução não realizar um desvio.

Nas instruções `JMP`, `JL`, `JE` e `JG`, o `PC` pode receber diretamente o endereço do destino.

---

# 11. Estados de execução

A UC possui os seguintes modos:

```text
CARGA
PARADO
STEP
AUTO
WAIT
HALT
ERRO
```

### CARGA

Recebe e monta instruções Assembly.

### PARADO

Programa carregado, mas sem execução ativa.

### STEP

Executa uma instrução somente após o comando `STEP` ou `*`.

### AUTO

Executa continuamente as instruções armazenadas.

### WAIT

A execução está temporariamente suspensa. O monitor serial continua sendo atendido.

### HALT

O programa terminou normalmente pela instrução `HALT`.

### ERRO

A execução foi interrompida devido a uma condição inválida, como opcode desconhecido, acesso a posição não carregada, `CMP` sem leitura válida ou desvio condicional sem comparação válida.

---

# 12. Comandos do monitor serial

Os comandos do ambiente não fazem parte da ISA e não ocupam posições da memória de programa.

| Comando | Função |
|---|---|
| `LOAD` | Limpa programa, memória, registradores e saídas e inicia nova carga |
| `END` | Finaliza a carga e valida os destinos dos desvios |
| `RUN` | Inicia execução passo a passo a partir de `0x00` |
| `STEP` | Executa uma instrução no modo passo a passo |
| `*` | Sinônimo de `STEP` |
| `AUTO` | Executa o programa continuamente a partir de `0x00` |
| `STOP` | Interrompe a execução/espera e desliga bombas e alarmes |
| `MEM` | Exibe todas as posições da memória |
| `MEM 0x00 0x0F` | Exibe um intervalo da memória |
| `STATUS` | Exibe registradores, flags, modo, níveis e saídas |

---

# 13. Ocupação da memória

A memória possui 128 posições, portanto o maior programa possível possui 128 instruções carregadas, desde que todas as posições sejam utilizadas.

Exemplo:

```text
0x00 → primeira instrução
0x01 → segunda instrução
0x02 → terceira instrução
...
0x7F → 128ª instrução
```

A posição `0x80` não existe na arquitetura.

Quando todas as 128 posições estiverem ocupadas, uma nova instrução deve ser rejeitada.

A variável:

```c
instrucao_atual
```

atua como ponteiro de carga. O valor `128` representa memória cheia no controle interno do programa.

---

# 14. Tratamento de erros

O projeto prevê erros tanto durante a montagem quanto durante a execução.

### Erros de montagem

- mnemônico inexistente;
- bomba fora de `1..3`;
- percentual fora de `0..100`;
- endereço sem `0x` ou fora de `0x00..0x7F`;
- quantidade incorreta de operandos;
- operandos extras;
- memória cheia.

Uma instrução rejeitada não deve alterar a memória nem avançar o ponteiro de carga.

### Erros de execução

- opcode inválido;
- campos incompatíveis com a ISA;
- tentativa de executar posição não carregada;
- `CMP` sem leitura válida;
- desvio condicional sem comparação válida;
- destino de desvio não carregado;
- término da memória sem `HALT` ou desvio válido.

Quando ocorre um erro de execução, o sistema entra em `MODO_ERRO` e desliga bombas e alarmes.

---

# 15. Representação do `ACC` em complemento de 2

Embora o `ACC` seja utilizado internamente como valor assinado, sua representação arquitetural pode ser exibida em 18 bits.

Para uma diferença negativa, os 18 bits seguem a representação em complemento de 2.

Exemplo:

```text
20 - 30 = -10
```

O valor decimal exibido é:

```text
ACC = -10
```

e sua representação binária pode ser apresentada com 18 bits.

Os 18 bits do ACC são independentes dos 18 bits da palavra de instrução: no caso do `ACC`, os bits representam um **valor assinado**; no caso da memória, representam **opcode + operandos**.

---

# 16. Relação entre Assembly, memória e execução

O projeto foi estruturado para que a sequência de processamento seja:

```text
                MONTAGEM
                   │
                   ▼
          ┌─────────────────┐
          │  TEXTO ASSEMBLY │
          └────────┬────────┘
                   │
                   ▼
              ASSEMBLER
                   │
                   ▼
          ┌─────────────────┐
          │ PALAVRA 18 BITS │
          └────────┬────────┘
                   │
                   ▼
              MEM_PROG
                   │
                   ▼
              ┌────────┐
              │   PC   │
              └────┬───┘
                   │
                   ▼
              ┌────────┐
              │  MAR   │
              └────┬───┘
                   │
                   ▼
              ┌────────┐
              │ MEMÓRIA│
              └────┬───┘
                   │
                   ▼
              ┌────────┐
              │  MBR   │
              └────┬───┘
                   │
                   ▼
              ┌────────┐
              │   IR   │
              └────┬───┘
                   │
                   ▼
              ┌─────────┐
              │ DECODER │
              └────┬────┘
                   │
                   ▼
              ┌─────────┐
              │   UC    │
              └────┬────┘
                   │
          ┌────────┼────────┐
          ▼        ▼        ▼
         ULA       E/S      PC
          │        │
          ▼        ▼
        ACC/     Bombas /
        FLAGS    Alarmes /
                 Display
```

O Assembly é utilizado para **montar** o programa, mas não para determinar diretamente a operação durante a execução. Durante a execução, a fonte de informação da CPU é a palavra armazenada em `MEM_PROG`.

---

# 17. Decisões arquiteturais principais

| Decisão | Justificativa |
|---|---|
| Palavra de 18 bits | É exatamente `4 + 7 + 7`, cobrindo opcode, dois operandos e o formato `CMP` |
| Opcode de 4 bits | Representa até 16 códigos; 15 são utilizados e 1 fica reservado para inválido |
| Operandos de 7 bits | Permitem representar 128 valores, coincidindo com o espaço de endereçamento |
| Memória de 128 posições | Corresponde diretamente aos 7 bits de endereço e ao intervalo `0x00..0x7F` |
| Uma instrução por posição | Mantém o tamanho fixo e simplifica busca e execução |
| `uint32_t` para palavra armazenada | Facilita as operações de deslocamento e máscara; somente 18 bits têm significado arquitetural |
| `MEM_OCUPADA` separado | Evita confundir uma palavra zerada com uma posição que não foi carregada |
| `IR` com a palavra inteira | Permite que a decodificação seja feita diretamente sobre a instrução armazenada |
| `ACC` assinado | Necessário para representar diferenças negativas em `CMP` |
| ULA separada da UC | Mantém a distinção entre cálculo e controle |
| `WAIT` não bloqueante | Permite continuar atendendo o monitor serial durante a espera |

---

# 18. Resumo quantitativo da arquitetura

```text
ISA
├── 15 instruções válidas
├── 4 bits de opcode
├── 7 bits de operando 1
└── 7 bits de operando 2

PALAVRA
└── 18 bits

MEMÓRIA
├── 7 bits de endereço
├── 128 posições
├── 0x00 .. 0x7F
├── 18 bits por posição
├── 2304 bits de capacidade lógica
└── 288 bytes de capacidade lógica

IMPLEMENTAÇÃO DE MEM_PROG
├── 128 elementos
├── uint32_t por elemento
└── 512 bytes de armazenamento do array

REGISTRADORES PRINCIPAIS
├── PC
├── MAR
├── MBR
├── IR
├── ACC
├── OP1 / OP2
└── FLAGS
```

---

# 19. Exemplo de codificação

Considere:

```asm
CMP 1, 30
```

O opcode de `CMP` é:

```text
1000
```

A bomba `1` em 7 bits é:

```text
0000001
```

O percentual `30` em 7 bits é:

```text
0011110
```

Logo, a palavra é:

```text
1000 0000001 0011110
```

Total:

```text
4 + 7 + 7 = 18 bits
```

Outro exemplo:

```asm
JMP 0x2A
```

O destino hexadecimal `0x2A` corresponde a decimal `42`, que em 7 bits é:

```text
0101010
```

Logo:

```text
1001 0101010 0000000
```

---

# 20. Conclusão

A arquitetura foi organizada para representar, de forma explícita, os principais conceitos de um computador clássico: **memória de programa, registradores, unidade de controle, unidade lógica e aritmética, decodificação de instruções e ciclo de busca/execução**.

A escolha de **18 bits por palavra** é sustentada pela estrutura da ISA:

```text
4 bits de opcode + 7 bits + 7 bits = 18 bits
```

Os **7 bits de endereço** permitem exatamente **128 posições de memória**, de `0x00` a `0x7F`. Como cada posição possui uma palavra de 18 bits, a capacidade lógica da memória é:

```text
128 × 18 = 2304 bits = 288 bytes
```

A implementação utiliza `uint32_t` para armazenar cada palavra por praticidade, mas os 14 bits restantes não fazem parte da memória arquitetural. A separação entre conteúdo (`MEM_PROG`) e ocupação (`MEM_OCUPADA`) também permite detectar corretamente posições não carregadas.

O resultado é uma máquina virtual simples, de palavra fixa, com espaço de endereçamento definido, ISA própria e ciclo de instrução explícito, adequada à proposta do trabalho de aplicar os conceitos fundamentais de Arquitetura de Computadores.
