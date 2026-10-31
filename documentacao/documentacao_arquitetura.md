# Documentação da Arquitetura — Sistema Programável de Três Bombas (com SECDED)

## 1. Visão geral

Este projeto implementa, sobre um **Arduino Mega 2560**, uma arquitetura de computador simulada para monitoramento e controle de três reservatórios industriais.

A aplicação física utiliza três potenciômetros como entradas analógicas (níveis dos reservatórios), três atuadores de bomba, alarme sonoro (buzzer), dois displays de 7 segmentos para telemetria numérica de 00% a 99% e **três LEDs RGB** para monitoramento contínuo do nível volumétrico dos tanques. O programa que controla o sistema é escrito em uma **ISA própria**, montado e carregado via monitor serial.

Para garantir alta confiabilidade e tolerância a falhas na memória simulada, foi incorporado um controlador de código de correção de erros com tecnologia **SECDED (*Single Error Correction, Double Error Detection*)** baseado no código de Hamming Estendido (24,18).

A execução segue o modelo clássico de **programa armazenado com ECC e *memory scrubbing***:

```text
Assembly
   ↓
ASSEMBLER
   ↓
Palavra lógica da ISA (18 bits)
   ↓
Codificador SECDED (Hamming 24,18)
   ↓
MEM_PROG (Palavra física de 24 bits)
   ↓
Busca (MAR → Leitura física)
   ↓
Decodificador & Verificador SECDED ──[Erro 1 bit]──> Scrubbing (auto-correção na RAM)
   ↓                                ──[Erro 2 bits]─> Interrupção / MODO_ERRO (DED)
MBR (18 bits lógicos corrigidos)
   ↓
IR
   ↓
DECODER
   ↓
UC
   ↓
ULA / E-S / PC / ACC / FLAGS

```

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
              ┌────────────────────────────┼────────────────────────────┐
              │                            │                            │
             ULA                    SUBSISTEMA MEMÓRIA                 E/S
              │                     ┌──────────────────┐                │
        ACC / FLAGS                 │ Controlador ECC  │        Entradas / Saídas
                                    │ (SECDED 24,18)   │                │
                                    └────────┬─────────┘                │
                                             │                          │
                                    MEM_PROG (24 bits)                  │
                                             │                          │
                                   MAR / MBR / MBR_RAW                  │
                                             │                          │
                                            IR                          │
                                             │                          │
                                            PC ─────────────────────────┘

```

### 2.1 Unidade de Controle (UC)

A UC coordena o ciclo de instrução e monitora a integridade de dados:

1. **BUSCA:** carrega o endereço do `PC` no `MAR` e invoca `MEMORIA(MEM_LEITURA)`. Se for detectado erro irrecuperável de 2 bits (DED), a UC aborta imediatamente a execução e transiciona para `MODO_ERRO`. Havendo erro simples (SEC), ele é corrigido transparentemente, a memória é reparada (*scrubbing*) e o valor decodificado segue para o `MBR`.
2. **DECODIFICAÇÃO:** transfere a palavra lógica do `MBR` para o `IR` e extrai opcode e operandos através do `DECODER`.
3. **EXECUÇÃO:** executa a operação na ULA, nas saídas de hardware ou no fluxo de controle (`PC`), atualizando os registradores de estado.

### 2.2 Decoder

Recebe os 18 bits de `IR`:

```text
IR[17..14] → OPCODE (4 bits)
IR[13..7]  → OPERANDO 1 (7 bits)
IR[6..0]   → OPERANDO 2 (7 bits)

```

### 2.3 Unidade Lógica e Aritmética (ULA)

Executa a subtração utilizada por `CMP`:

$$\text{ACC} = \text{NIVEL}[x] - n$$

Atualizando os sinalizadores `FLAG_L` ($<0$), `FLAG_Z` ($=0$) e `FLAG_G` ($>0$).

### 2.4 Subsistema de Memória e Controlador SECDED

A interface da memória é responsável pelo isolamento entre a representação lógica da ISA (18 bits) e a representação física no armazenamento (24 bits). O controlador intercepta todos os acessos:

* **Na escrita (`MEM_ESCRITA`):** gera os 5 bits de paridade de Hamming e 1 bit de paridade global, gravando a palavra física de 24 bits.
* **Na leitura (`MEM_LEITURA`):** calcula o vetor de síndrome e a paridade total. Se encontrar um bit corrompido, inverte o bit para corrigi-lo, reescreve a palavra corrigida na memória física (*memory scrubbing*) e incrementa `CONTADOR_SEC`. Se detectar 2 bits invertidos, gera alarme de erro crítico, incrementa `CONTADOR_DED` e impede a instrução de executar.

---

## 3. Definição da palavra da ISA e Proteção SECDED

### 3.1 Palavra lógica da ISA (18 bits)

A ISA opera sobre uma palavra lógica de **18 bits**, invariável para todas as instruções:

```text
17                         14 13                       7 6             0
┌────────────────────────────┬───────────────────────────┬───────────────┐
│          OPCODE            │       OPERANDO 1          │  OPERANDO 2   │
│           4 bits           │          7 bits           │    7 bits     │
└────────────────────────────┴───────────────────────────┴───────────────┘

```

* **4 bits de Opcode:** comportam as 15 instruções ativas (`0000` a `1110`) e 1 código reservado/inválido (`1111`).
* **7 bits de Operando 1:** suportam endereços de 0 a 127 (`0x00` a `0x7F`), identificação de bomba/reservatório (`1..3`) ou tempos de espera `WAIT` (`0..127 ticks` = `0..12,7 s`).
* **7 bits de Operando 2:** suportam percentuais de 0 a 100 (`CMP`) ou preenchimento com zeros.

### 3.2 Palavra física de armazenamento com SECDED (24 bits)

Para proteger a palavra de 18 bits contra corrupção por radiação ou ruído elétrico (*single-event upsets* / *bit-flips*), implementou-se o código **Hamming Estendido $(24, 18)$**, adicionando 6 bits de redundância (5 bits de paridade intercalados + 1 bit de paridade global):

$$\text{Bits físicos} = 18 \text{ bits dados} + 5 \text{ bits Hamming } (P_1, P_2, P_4, P_8, P_{16}) + 1 \text{ bit Global } (P_0) = 24 \text{ bits}$$

#### Mapeamento de posições físicas (1 a 23 + bit 0):

* **Bit 0 ($P_0$):** Paridade global de paridade par, calculada sobre todas as posições $1..23$.
* **Potências de 2 ($1, 2, 4, 8, 16$):** Bits de paridade do código de Hamming ($P_1, P_2, P_4, P_8, P_{16}$).
* **Demais posições:** Os 18 bits de dados da ISA ($D_0$ a $D_{17}$), mapeados conforme a tabela:

| Bit ISA | $D_0$ | $D_1$ | $D_2$ | $D_3$ | $D_4$ | $D_5$ | $D_6$ | $D_7$ | $D_8$ | $D_9$ | $D_{10}$ | $D_{11}$ | $D_{12}$ | $D_{13}$ | $D_{14}$ | $D_{15}$ | $D_{16}$ | $D_{17}$ |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **Pos. Física** | 3 | 5 | 6 | 7 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 17 | 18 | 19 | 20 | 21 | 22 | 23 |

### 3.3 Mecanismo de Diagnóstico e Correção

O controlador avalia dois fatores no ciclo de leitura:

1. **Síndrome ($S$ - 5 bits):** calculada verificando as paridades de grupo nas posições $1..23$.
2. **Paridade Global Total ($P_{\text{total}}$):** paridade de todos os 24 bits.

| $S$ | $P_{\text{total}}$ | Classificação | Ação do Sistema |
| --- | --- | --- | --- |
| $= 0$ | $0$ | **ECC_OK** | Nenhuma inversão. Dados lidos diretamente. |
| $\neq 0$ (ou $S=0$) | $1$ | **ECC_SEC_CORRIGIDO** | **Erro Simples (1 bit):** A síndrome indica exatamente a posição física do bit invertido ($1..23$). Se $S=0$, o erro foi no bit $P_0$. O bit é invertido em hardware simulado, corrigido em `MBR` e regravado em `MEM_PROG[MAR]` (*memory scrubbing*). |
| $\neq 0$ | $0$ | **ECC_DED_ERRO_DUPLO** | **Erro Duplo Detectado (2 bits):** Falha crítica irrecuperável. A leitura falha, a UC interrompe a execução com erro e o processador entra em `MODO_ERRO`. |

---

## 4. Tamanho e capacidade da memória

### 4.1 Quantidade de posições e endereçamento

A memória possui **128 posições** indexadas de `0x00` a `0x7F` (7 bits de endereço).

### 4.2 Capacidade Lógica vs. Física

* **Capacidade Lógica da ISA:**

$$128 \times 18 \text{ bits} = 2304 \text{ bits} = 288 \text{ bytes de dados úteis}$$


* **Capacidade Física Efetiva (com SECDED):**

$$128 \times 24 \text{ bits} = 3072 \text{ bits} = 384 \text{ bytes físicos}$$


* **Armazenamento em C++ (`uint32_t MEM_PROG[128]`):**

$$128 \times 32 \text{ bits} = 512 \text{ bytes}$$



---

## 5. Registradores e Estados de Hardware

| Registrador / Variável | Largura / tipo | Função |
| --- | --- | --- |
| `PC` | 7 bits | Contador de programa (próxima instrução) |
| `MAR` | 7 bits | Registrador de endereço de memória |
| `MBR` | 18 bits | Registrador de dados da memória (palavra lógica da ISA decodificada) |
| `MBR_RAW` | 24 bits | Palavra física lida/escrita na memória (inclui bits SECDED) |
| `IR` | 18 bits | Registrador de instrução em execução |
| `ECC_SINDROME` | 5 bits | Síndrome calculada no último acesso (0..23) |
| `ECC_PARIDADE_GLOBAL` | 1 bit | Paridade total calculada no último acesso |
| `ECC_STATUS` | Enum | Estado do SECDED (`OK`, `SEC_CORRIGIDO`, `DED_ERRO_DUPLO`) |
| `CONTADOR_SEC` | 16 bits | Total acumulado de erros simples corrigidos desde o último `LOAD` |
| `CONTADOR_DED` | 16 bits | Total acumulado de erros duplos detectados desde o último `LOAD` |
| `ACC` | `int16_t` | Acumulador assinado (armazena leituras e resultados da ULA) |
| `OPCODE_REG` | 4 bits | Código da operação decodificada |
| `OP1`, `OP2` | 7 bits cada | Operandos decodificados da instrução |
| `FLAGS (L, Z, G)` | 1 bit cada | Indicadores condicionais da ULA ($<, =, >$) |
| `LED_HABILITADO[3]` | `bool[3]` | Habilitação do monitoramento visual contínuo por LED RGB de cada reservatório |
| `BOMBA_LIGADA[3]` | `bool[3]` | Estado operacional dos motores das bombas (1..3) |
| `ALARME_CORRETIVO[3]` | `bool[3]` | Sinalizador de manutenção corretiva e acionamento de alarme acústico |

---

## 6. Subsistema de Telemetria Visual (LEDs RGB e displays)

O monitoramento visual analógico conta com **três LEDs RGB** independentes, um para cada reservatório. Cada LED utiliza os canais Vermelho (**R**) e Verde (**G**) para exibir três estados contínuos de volumetria:

| Faixa de Volume | Cor Exibida | Canal R | Canal G | Significado Operacional |
| --- | --- | --- | --- | --- |
| $\text{NÍVEL} \le 20\%$ | **Vermelho** | `HIGH` | `LOW` | **Nível Crítico:** Risco de desabastecimento / cavitação da bomba. |
| $21\% \le \text{NÍVEL} \le 70\%$ | **Amarelo** | `HIGH` | `HIGH` | **Nível Razoável:** Faixa nominal operacional intermediária. |
| $\text{NÍVEL} > 70\%$ | **Verde** | `LOW` | `HIGH` | **Nível Cheio:** Capacidade máxima ou próxima ao transbordamento. |
| Desabilitado / Desligado | **Apagado** | `LOW` | `LOW` | Monitoramento visual do tanque inativo. |

* **Atualização em Tempo Real:** Uma vez habilitado o indicador de um reservatório (`LED_HABILITADO[x - 1] = true`), qualquer leitura analógica realizada (`READ x` ou `INFO x`) recalcula e atualiza instantaneamente a cor do LED RGB correspondente.
* **Pinagem do hardware:** potenciômetros em `A0`, `A1` e `A2`; bombas em `46`, `47` e `48`; canais RGB vermelho/verde em `{38,40,42}` e `{39,41,43}`; buzzer em `52`.
* **Displays:** o display de dezenas usa os segmentos `a..g` nos pinos `{22,23,24,25,26,27,28}` e o display de unidades nos pinos `{29,30,31,32,33,34,35}`. Os displays são de ânodo comum e, por isso, os segmentos são acionados em nível baixo. A instrução `INFO x` mostra o percentual inteiro de `00` a `99`; leituras de `100%` são limitadas a `99` no display.

---

## 7. Formato e Conjunto de Instruções

A ISA possui formato fixo de 18 bits (`OPCODE [4] | OP1 [7] | OP2 [7]`):

| Mnemônico | Opcode | OP1 (7b) | OP2 (7b) | Descrição |
| --- | --- | --- | --- | --- |
| `READ x` | `0000` | Reservatório (1..3) | `0000000` | Lê nível analógico (0..100%), salva em `NIVEL[x]` e `ACC`, atualizando o LED RGB caso esteja habilitado. |
| `ON x` | `0001` | Bomba (1..3) | `0000000` | Aciona a bomba `x`. |
| `OFF x` | `0010` | Bomba (1..3) | `0000000` | Desliga a bomba `x`. |
| `ALARM x` | `0011` | Bomba (1..3) | `0000000` | Ativa alarme corretivo da bomba `x` e aciona buzzer. |
| `LED x` | `0100` | Reservatório (1..3) | `0000000` | Habilita o monitoramento contínuo por LED RGB do reservatório `x`, acendendo na cor do nível atual. |
| `INFO x` | `0101` | Reservatório (1..3) | `0000000` | Lê nível analógico, atualiza LED RGB, exibe o percentual de `00` a `99` nos dois displays e salva em `ACC`. |
| `SILENCE x` | `0110` | Bomba (1..3) | `0000000` | Desativa alarme corretivo da bomba `x`. |
| `LEDOFF x` | `0111` | Reservatório (1..3) | `0000000` | Desabilita o monitoramento visual do reservatório `x` e apaga seu LED RGB. |
| `CMP x,n` | `1000` | Reservatório (1..3) | Limite (0..100) | Calcula `NIVEL[x] - n` via ULA e atualiza `ACC` e flags `L`, `Z`, `G`. |
| `JMP a` | `1001` | Endereço (0x00..0x7F) | `0000000` | Desvio incondicional para `a`. |
| `JL a` | `1010` | Endereço (0x00..0x7F) | `0000000` | Desvia para `a` se `FLAG_L == 1`. |
| `JE a` | `1011` | Endereço (0x00..0x7F) | `0000000` | Desvia para `a` se `FLAG_Z == 1`. |
| `JG a` | `1100` | Endereço (0x00..0x7F) | `0000000` | Desvia para `a` se `FLAG_G == 1`. |
| `WAIT t` | `1101` | Ticks (0..127) | `0000000` | Pausa por $t \times 100\text{ ms}$ de forma não bloqueante. |
| `HALT` | `1110` | `0000000` | `0000000` | Encerra a execução mantendo estados atuais dos atuadores e LEDs. |

---

## 8. Comandos do Monitor Serial

| Comando | Descrição |
| --- | --- |
| `LOAD` | Reseta o sistema, limpa memória, apaga display e LEDs RGB, zera estatísticas SECDED e abre modo de carga. |
| `END` | Valida as instruções, verifica integridade SECDED da carga, destinos de saltos e fecha a montagem. |
| `RUN` | Prepara o sistema para o modo de execução passo a passo a partir de `0x00`. |
| `STEP` ou `*` | Executa um único ciclo de instrução (Busca → Decodifica → Executa) e emite telemetria. |
| `AUTO` | Executa o programa continuamente em alta velocidade. |
| `STOP` | Interrompe imediatamente qualquer execução ou temporização, desligando bombas, alarmes e LEDs RGB. |
| `STATUS` | Exibe o painel geral da CPU, registradores, atuadores, estado dos 3 LEDs RGB (`VERMELHO`, `AMARELO`, `VERDE` ou `OFF`) e estatísticas SECDED. |
| `MEM [ini fim]` | Despeja a memória exibindo o endereço, a palavra física SECDED (24 bits), a palavra lógica ISA (18 bits) e o diagnóstico ECC. |
| `FLIP <end> <bit>` | **Injetor de falhas:** inverte o bit físico especificado (`0..23`) no endereço indicado para validar a correção SEC ou parada por DED. |

---

## 9. Exemplo Prático de Injeção de Falha e Diagnóstico

1. O montador grava uma instrução na memória:

```text
Montado em 0x00 | ISA[18b]=000000000010000000 | SECDED[24b]=000000001000010100000100

```

2. O usuário injeta uma falha de bit simples na RAM via monitor serial:

```text
FLIP 0x00 10

```

3. O comando inverte o bit físico 10 e acusa a falha esperada:

```text
FLIP aplicado em 0x00 no bit fisico 10 | SECDED[24b]=... | Diagnostico esperado: SEC (1 Erro Corrigido) (S=10, P0=1)

```

4. Durante a busca (`UC` / `STEP`), a memória intercepta:

```text
[SECDED-SEC] 1 bit invertido detectado e corrigido em 0x00 | Bit fisico corrigido (Sindrome)=10 | P_total=1

```

A palavra é automaticamente corrigida em tempo de execução (*scrubbing*), a execução da ISA prossegue sem falhas e o contador `CONTADOR_SEC` é incrementado.

5. Caso o usuário injete um segundo erro no mesmo endereço com outro `FLIP`, a leitura do ciclo seguinte detecta:

```text
[SECDED-DED] ERRO DUPLO INrecuperavel em 0x00 | Sindrome=... | P_total=0
ERRO DE EXECUCAO: SECDED detectou ERRO DUPLO (DED) na memoria de programa

```

O processador paralisa imediatamente a máquina e desliga os atuadores e sinalizadores, prevenindo falhas no processo físico.
