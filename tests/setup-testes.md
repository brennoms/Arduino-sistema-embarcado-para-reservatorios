# Roteiro e Conjunto de Instruções de Teste — Arquitetura de Três Bombas

Este documento apresenta as sequências de comandos para o monitor serial (115200 bps), o código Assembly, os comportamentos esperados de hardware/telemetria e as instruções de execução para cada um dos itens da Seção 4 (Setup de Testes).

---

## 4.1 Montagem e Programa Armazenado

### Teste 4.1.1 — Carga Básica e Integridade de Endereçamento

* **Entrada (Monitor Serial):**

```text
LOAD
READ 1
ON 1
OFF 1
HALT
END
MEM 0x00 0x03

```

* **Comportamento Esperado:**
* O comando `LOAD` zera registradores e limpa a memória.


* Nenhuma saída física (bombas, LEDs RGB ou buzzer) é acionada durante a digitação das instruções.


* Cada instrução é montada sequencialmente nas posições `0x00`, `0x01`, `0x02` e `0x03`.


* `END` valida a integridade e encerra o modo de carga sem disparar a execução.


* A consulta `MEM 0x00 0x03` exibe as 4 palavras lógicas de 18 bits e as palavras físicas de 24 bits (SECDED) intactas (`ECC_OK`).




* **Decodificação dos Campos de uma Palavra (Exemplo: `READ 1` em `0x00`):**
* Palavra lógica ISA (18 bits): `0000 0000001 0000000`

* **Opcode (bits 17..14 = `0000`):** Código da instrução `READ`.


* **Operando 1 (bits 13..7 = `0000001`):** Identificação da Bomba 1 (`0x01`).


* **Operando 2 (bits 6..0 = `0000000`):** Sem uso (zerado).







### Teste 4.1.2 — Robustez do Parser (Comentários e Espaços)

* **Entrada (Monitor Serial):**

```text
LOAD
; Linha contendo apenas comentário inicial

READ 1 ; Le o nivel do tanque 1

   ON 1    ; Aciona a bomba 1 com espacos antes e depois
; Outro comentario intermediario
OFF 1
HALT
END
MEM 0x00 0x03

```

* **Comportamento Esperado:**
* O montador descarta linhas em branco e comentários iniciados por `;`.


* Os endereços gerados (`0x00` a `0x03`) e as palavras binárias de 18 e 24 bits devem ser estritamente idênticos aos obtidos no Teste 4.1.1.





---

## 4.2 Ciclo de Instrução

* **Carga Inicial:** Utilizar o programa carregado no Teste 4.1 (`READ 1`, `ON 1`, `OFF 1`, `HALT`).


* **Entrada (Monitor Serial):**

```text
RUN
STEP
STEP
STEP
STEP

```

* **Comportamento e Rastreamento Esperado:**
* `RUN`: Prepara o sistema, apontando `PC = 0x00` em modo passo a passo sem alterar saídas.


* **Passo 1 (`STEP` em `0x00` - `READ 1`):**
* *Busca:* `MAR` recebe `0x00`; `MEMORIA(MEM_LEITURA)` valida paridade SECDED e carrega palavra em `MBR`.


* *Decodificação:* `IR` recebe `MBR` (`000000000010000000`); `DECODER` extrai `OPCODE=0` (`READ`), `OP1=1`.


* *Execução:* Realiza a leitura analógica de A0, armazena em `NIVEL[0]` e `ACC`; próximo `PC = 0x01`.




* **Passo 2 (`STEP` em `0x01` - `ON 1`):**
* `IR` decodifica `OPCODE=1` (`ON`), `OP1=1`; pino 46 vai para nível alto (`HIGH`); próximo `PC = 0x02`.




* **Passo 3 (`STEP` em `0x02` - `OFF 1`):**
* `IR` decodifica `OPCODE=2` (`OFF`), `OP1=1`; pino 46 volta para nível baixo (`LOW`); próximo `PC = 0x03`.




* **Passo 4 (`STEP` em `0x03` - `HALT`):**
* `IR` decodifica `OPCODE=14` (`HALT`); o processador transiciona para `MODO_HALT` e encerra o avanço de `PC`.







---

## 4.3 Leitura Independente

* **Entrada (Monitor Serial):**

```text
LOAD
READ 1
READ 2
READ 3
HALT
END
RUN

```

* **Procedimento de Teste:**
1. Girar o potenciômetro da Bomba 1 (A0) para o fim de curso mínimo (0V).


2. Ajustar o potenciômetro da Bomba 2 (A1) para uma posição intermediária (aprox. 50%).


3. Girar o potenciômetro da Bomba 3 (A2) para o fim de curso máximo (5V).


4. Executar os ciclos:



```text
STEP
STEP
STEP
STATUS

```

* **Comportamento Esperado:**
* Após o 1º `STEP`: `NIVEL[0] = 0%`, `ACC = 0` (`ACC[18b]=000000000000000000`).


* Após o 2º `STEP`: `NIVEL[1] = 50%`, `ACC = 50`.


* Após o 3º `STEP`: `NIVEL[2] = 100%`, `ACC = 100`.


* O comando `STATUS` comprova a independência das três variáveis analógicas no registrador `NIVEIS: [0%, 50%, 100%]`.





---

## 4.4 Comparação e Desvios

### Programa de Teste de Flags e Complemento de 2

* **Código Assembly:**

```text
LOAD
READ 1      ; 0x00 - Suponha nivel lido em 20%
CMP 1, 30   ; 0x01 - Calcula 20 - 30 = -10 (Ativa FLAG_L)
JL 0x05     ; 0x02 - Deve desviar para 0x05
HALT        ; 0x03 - Nao deve executar
HALT        ; 0x04 - Nao deve executar
CMP 1, 20   ; 0x05 - Calcula 20 - 20 = 0 (Ativa FLAG_Z)
JE 0x09     ; 0x06 - Deve desviar para 0x09
HALT        ; 0x07 - Nao deve executar
HALT        ; 0x08 - Nao deve executar
CMP 1, 10   ; 0x09 - Calcula 20 - 10 = +10 (Ativa FLAG_G)
JG 0x0D     ; 0x0A - Deve desviar para 0x0D
HALT        ; 0x0B - Nao deve executar
HALT        ; 0x0C - Nao deve executar
HALT        ; 0x0D - Fim com sucesso
END
RUN

```

* **Comportamento Esperado e Análise Aritmética:**
* Ajustar o potenciômetro 1 em **20%**.


* Executar via `AUTO` ou pulsos de `STEP` sucessivos.


* Na instrução `0x01` (`CMP 1, 30`):


* A ULA calcula: $\text{ACC} = 20 - 30 = -10$.


* Flags atualizadas: `L=1`, `Z=0`, `G=0`.


* **Complemento de 2 (18 bits):**
* $+10_{10} = \text{00 0000 0000 0000 1010}_2$
* Inversão de bits: $\text{11 1111 1111 1111 0101}_2$
* Soma 1: $\text{11 1111 1111 1111 0110}_2$ (`0x3FFF6`).


* O trace serial confirmará `ACC decimal=-10` e `ACC[18b]=111111111111110110`.




* O comando subsequente `JL 0x05` avalia `FLAG_L == 1` e força `PC = 0x05`, saltando `0x03` e `0x04`.




* Na instrução `0x05` (`CMP 1, 20`): $\text{ACC} = 0$, flags: `L=0`, `Z=1`, `G=0`; `JE 0x09` desvia para `0x09`.


* Na instrução `0x09` (`CMP 1, 10`): $\text{ACC} = +10$, flags: `L=0`, `Z=0`, `G=1`; `JG 0x0D` desvia diretamente para `0x0D` (`HALT`).





---

## 4.5 Bombas e Alarmes

* **Entrada (Monitor Serial):**

```text
LOAD
ON 1
LED 1
ON 2
LED 2
ON 3
LED 3
ALARM 1
ALARM 2
SILENCE 1
SILENCE 2
OFF 1
LEDOFF 1
OFF 2
LEDOFF 2
OFF 3
LEDOFF 3
HALT
END
RUN

```

* **Roteiro de Validação Física:**
1. Executar passo a passo até acionar `ON 1..3` e `LED 1..3`:
* Os pinos das bombas (46, 47, 48) vão a nível alto sequencialmente.


* Os LEDs RGB 1, 2 e 3 acendem na cor correspondente ao nível de cada tanque (Vermelho, Amarelo ou Verde).




2. Executar `ALARM 1` seguido de `ALARM 2`:
* O buzzer no pino 52 é ativado com `ALARM 1`.


* Com `ALARM 2`, o buzzer permanece soando continuamente.




3. Executar `SILENCE 1`:
* `ALARME_CORRETIVO[0]` é desmarcado, porém o **buzzer permanece ligado** pois `ALARME_CORRETIVO[1]` continua ativo.




4. Executar `SILENCE 2`:
* Não restando alarmes pendentes no sistema, a função `atualizarBuzzer()` **desliga o buzzer** (pino 52 vai para `LOW`).




5. As instruções finais desativam as bombas e apagam todos os canais dos LEDs RGB (`RGB_OFF`).





---

## 4.6 Displays de 7 Segmentos e Telemetria

* **Entrada (Monitor Serial):**

```text
LOAD
INFO 1
INFO 2
INFO 3
WAIT 10
JMP 0x00
END
RUN

```

* **Verificação dos dois dígitos (Display e Serial):**
A instrução `INFO x` primeiro mostra a identificação do reservatório por 1 segundo e depois mostra o percentual inteiro:
* Identificação: `L` no display de dezenas e o número do reservatório no display de unidades (`L1`, `L2` ou `L3`). O nível é lido no início da instrução, antes da exibição da identificação.
* O percentual é mantido nos dois displays de ânodo comum após a identificação:
* Dezenas: segmentos `a..g` nos pinos `22, 23, 24, 25, 26, 27, 28` (`DISP1`).
* Unidades: segmentos `a..g` nos pinos `29, 30, 31, 32, 33, 34, 35` (`DISP2`).
* Para ânodo comum, um segmento aceso deve estar em `LOW` e um segmento apagado em `HIGH`.

| Nível (%) | Display de dezenas | Display de unidades | Retorno Serial (`INFO`) |
| --- | --- | --- | --- |
| **0%** | `0` | `0` | `INFO bomba x -> 0% | display=0%` |
| **7%** | `0` | `7` | `INFO bomba x -> 7% | display=7%` |
| **42%** | `4` | `2` | `INFO bomba x -> 42% | display=42%` |
| **99%** | `9` | `9` | `INFO bomba x -> 99% | display=99%` |
| **100%** | `9` | `9` | `INFO bomba x -> 100% | display=99%` |

* **Procedimento de Teste de Transição:**
* Ajustar o potenciômetro para aproximadamente `0%`, `7%`, `42%`, `99%` e `100%`.


* Executar `STEP` após cada ajuste e confirmar `Lx` por 1 segundo, seguido pelos dois dígitos do percentual no display físico e pelo valor exibido no log do terminal serial.





---

## 4.7 Memória e Endereçamento

### Teste 4.7.1 — Leituras e Não-Volatilidade de Execução

* **Entrada (Monitor Serial):**

```text
MEM
MEM 0x00 0x0A
MEM 0x70 0x7F
STATUS

```

* **Comportamento Esperado:**
* `MEM` exibe a listagem completa (0x00 a 0x7F) com os bits físicos SECDED de 24 bits e os bits decodificados da ISA (18 bits).


* Comandos de leitura de memória não modificam os registradores da CPU (`PC`, `ACC`, `FLAGS`) e mantêm as saídas inalteradas.





### Teste 4.7.2 — Esgotamento da Memória (128 Posições)

* **Procedimento:** Enviar via script ou bloco de repetição serial um lote de 128 instruções `NOP` simuladas (por exemplo, `WAIT 0` repetido 128 vezes do endereço `0x00` até `0x7F`):



```text
LOAD
WAIT 0
; ... [repetir ate preencher 128 instrucoes] ...
WAIT 0

```

* **Tentativa de Inserir a 129ª Instrução:**

```text
WAIT 0

```

* **Resultado Esperado:**
* O montador rejeita imediatamente a operação e emite no monitor serial:




```text
ERRO: memoria de programa cheia.

```



---

## 4.8 Tratamento de Erros e Injeção de Falhas

Submeta individualmente as seguintes entradas anômalas para validar os mecanismos de proteção da arquitetura:

1. **Instrução Desconhecida / Mnemônico Inválido:**
* Entrada: `LOAD` seguido de `XYZ 1`

* Resultado: `ERRO: instrucao ou operandos invalidos.`



2. **Bomba Inválida:**
* Entrada: `ON 4` ou `READ 0`

* Resultado: `ERRO: instrucao ou operandos invalidos.` (rejeitado pelo parser no intervalo 1..3).




3. **Endereço em Decimal no Desvio:**
* Entrada: `JMP 12` (em vez do formato hexadecimal `0x0C`)


* Resultado: `ERRO: instrucao ou operandos invalidos.`



4. **Intervalo Invertido no Monitor:**
* Entrada: `MEM 0x10 0x05`

* Resultado: `ERRO: intervalo invertido.`



5. **Comparação Sem Leitura Prévia:**
* Sequência: `LOAD` $\rightarrow$ `CMP 1, 50` $\rightarrow$ `END` $\rightarrow$ `RUN` $\rightarrow$ `STEP`

* Resultado: `ERRO DE EXECUCAO: CMP sem leitura valida para a bomba indicada` (CPU para em `MODO_ERRO`).




6. **Desvio Condicional Sem Comparação:**
* Sequência: `LOAD` $\rightarrow$ `JL 0x02` $\rightarrow$ `HALT` $\rightarrow$ `END` $\rightarrow$ `RUN` $\rightarrow$ `STEP`

* Resultado: `ERRO DE EXECUCAO: desvio condicional sem comparacao valida`.




7. **Execução em Posição Não Carregada:**
* Sequência: `LOAD` $\rightarrow$ `JMP 0x20` (onde `0x20` está vazia)


* Resultado: O comando `END` detecta na validação estática: `ERRO: destino 0x20 da instrucao em 0x00 nao corresponde a uma posicao carregada.` impedindo o início da execução.




8. **Injeção de Falha na Memória com SECDED (Bit-Flip):**
* Sequência:
```text
LOAD
READ 1
HALT
END
FLIP 0x00 10
RUN
STEP

```


* Resultado:
* O comando `FLIP 0x00 10` inverte o bit físico 10.


* Na busca, o hardware intercepta a falha, corrige o bit via Hamming, reescreve na memória física (*scrubbing*), incrementa `CONTADOR_SEC` e prossegue normalmente sem interrupção.


* Ao aplicar um segundo flip (`FLIP 0x00 12`), o próximo acesso sinaliza `[SECDED-DED] ERRO DUPLO`, abortando a CPU para `MODO_ERRO` e desligando todas as bombas.







---

## 4.9 Controle Automático dos Três Reservatórios

### Descrição do Programa

Lê ciclicamente os tanques 1, 2 e 3. Se o nível estiver **abaixo de 30%**, a bomba é ligada (`ON`). Se estiver **acima de 80%**, a bomba é desligada (`OFF`). Entre 30% e 80%, o estado anterior é preservado. Os LEDs RGB mantêm o monitoramento do nível ativado.

### Código Assembly Completo

```text
LOAD
; Habilita LEDs RGB dos 3 reservatorios
LED 1       ; 0x00
LED 2       ; 0x01
LED 3       ; 0x02

; ================== BOMBA 1 ==================
READ 1      ; 0x03 - Le nivel do tanque 1
CMP 1, 30   ; 0x04 - Compara com 30%
JL 0x08     ; 0x05 - Se menor que 30%, liga bomba 1
CMP 1, 80   ; 0x06 - Compara com 80%
JG 0x0A     ; 0x07 - Se maior que 80%, desliga bomba 1
JMP 0x0B    ; 0x08 - Histerese (30..80%): mantem estado
ON 1        ; 0x09 - Liga Bomba 1
JMP 0x0B    ; 0x0A - Pula bloco de desligar
OFF 1       ; 0x0B - Desliga Bomba 1

; ================== BOMBA 2 ==================
READ 2      ; 0x0C - Le nivel do tanque 2
CMP 2, 30   ; 0x0D
JL 0x11     ; 0x0E
CMP 2, 80   ; 0x0F
JG 0x13     ; 0x10
JMP 0x14    ; 0x11
ON 2        ; 0x12
JMP 0x14    ; 0x13
OFF 2       ; 0x14

; ================== BOMBA 3 ==================
READ 3      ; 0x15 - Le nivel do tanque 3
CMP 3, 30   ; 0x16
JL 0x1A     ; 0x17
CMP 3, 80   ; 0x18
JG 0x1C     ; 0x19
JMP 0x1D    ; 0x1A
ON 3        ; 0x1B
JMP 0x1D    ; 0x1C
OFF 3       ; 0x1D

; ================= INTERVALO E LACO ==========
WAIT 10     ; 0x1E - Pausa de 1,0 s (10 x 100 ms) nao-bloqueante
JMP 0x03    ; 0x1F - Retorna ao ciclo de leitura
END

```

* **Procedimento de Teste:**
1. Digitar o programa e fechar com `END`.


2. Disparar a execução contínua com `AUTO`.


3. Mover o potenciômetro do Tanque 1 para **15%**:
* A Bomba 1 liga (`BOMBA 1 = 1`). O LED RGB 1 fica **Vermelho**.




4. Mover o potenciômetro gradualmente para **55%**:
* A Bomba 1 permanece ligada (região de histerese). O LED RGB 1 muda para **Amarelo**.




5. Mover o potenciômetro para **85%**:
* A Bomba 1 desliga (`BOMBA 1 = 0`). O LED RGB 1 muda para **Verde**.




6. Reduzir para **60%**:
* A Bomba 1 permanece desligada, demonstrando o funcionamento biestável.




7. **Interrupção:** Digitar `STOP` no monitor serial durante a temporização de `WAIT`:


* O sistema interrompe imediatamente o laço, zera as saídas e transiciona para `MODO_PARADO`.







---

## 4.10 HALT e Avaliação Conceitual

### Teste de Bloqueio Pós-HALT

* **Entrada (Monitor Serial):**

```text
LOAD
ON 1
HALT
END
RUN
STEP
STEP
STEP

```

* **Comportamento Esperado:**
* No 1º `STEP`: Executa `ON 1`, ligando a Bomba 1.


* No 2º `STEP`: Executa `HALT`, encerrando o programa mantendo a saída ligada (`MODO_HALT`).


* No 3º `STEP`: O comando é rejeitado com mensagem indicando término de execução; o `PC` não avança e nenhuma instrução é processada até novo `RUN` ou `AUTO`.





### Roteiro de Avaliação Conceitual da Arquitetura

Cada integrante deve demonstrar a localização e o papel dos módulos de hardware simulados no código-fonte:

1. **Montador (`ASSEMBLER` / `montarPalavra`):**
* Faz o parsing do texto da instrução, isolando mnemônico e operandos decimais/hexadecimais.


* Converte a sintaxe Assembly na palavra lógica da ISA de 18 bits (`OPCODE [4] | OP1 [7] | OP2 [7]`).




2. **Subsistema de Memória e SECDED (`MEM_PROG`, `codificarSECDED`, `decodificarSECDED`):**
* Intercepta a palavra lógica de 18 bits na escrita, calcula os 5 bits de Hamming e 1 bit de paridade global e salva a palavra física de 24 bits.


* Na leitura, detecta e corrige erros de 1 bit ou aborta se houver erro duplo irrecuperável.




3. **Ciclo de Instrução na Unidade de Controle (`UC`):**
* **Busca:** Coloca o endereço apontado por `PC` no registrador `MAR` e invoca `MEMORIA(MEM_LEITURA)`. A palavra física corrigida segue para `MBR_RAW` e os 18 bits úteis para `MBR`.


* **Decodificação (`DECODER`):** `IR` recebe o conteúdo de `MBR`. O decodificador extrai os campos por deslocamento de bits: `OPCODE = IR >> 14`, `OP1 = (IR >> 7) & 0x7F`, `OP2 = IR & 0x7F`.


* **Execução:** A UC chaveia a instrução pelo `switch(opcode)`, acionando os pinos de saída física ou invocando a ULA.




4. **ULA e Registradores (`ULA`, `ACC`, `FLAGS`):**
* Realiza a subtração `ULA_A - ULA_B` em complemento de 2 para instruções `CMP`.


* Atualiza os sinalizadores condicionais `FLAG_L`, `FLAG_Z` e `FLAG_G`, que balizam as decisões dos desvios condicionais (`JL`, `JE`, `JG`).