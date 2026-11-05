# Sistema programável de três reservatórios

Trabalho de Arquitetura de Computadores do IBMEC, orientado pelo professor Clayton J. A. Silva.

Três potenciômetros simulam o nível de água dos reservatórios. O Arduino Mega 2560 simula uma CPU que recebe um programa em assembly pelo monitor serial, carrega as instruções na memória e as executa. O programa controla as saídas das bombas e os alarmes.

## Objetivo e escopo

O projeto foi desenvolvido para demonstrar o funcionamento de um computador com programa armazenado. O Arduino Mega 2560 fornece as entradas e saídas do protótipo e executa o sketch que simula a CPU. A CPU simulada tem seu próprio conjunto de instruções, registradores e memória de programa. As instruções assembly deste trabalho são traduzidas pelo montador do sketch e interpretadas pela Unidade de Controle simulada.

O controle dos reservatórios é definido pelo programa carregado. O mesmo sketch pode executar uma demonstração curta, testar um alarme ou repetir continuamente a leitura dos três potenciômetros. O protótipo representa os níveis e os acionamentos por sinais elétricos. O comportamento observado nas saídas corresponde ao comando das bombas, dos LEDs e do buzzer.

## Arquivos do projeto

| Arquivo | Conteúdo |
| --- | --- |
| [Sketch do Arduino](../scripts/ArquiteturaSimulada.ino) | Montador, memória de programa, CPU simulada, interface serial e controle das entradas e saídas. |
| [Documentação da arquitetura](documentacao_arquitetura.md) | Detalhes da ISA, dos registradores e da proteção SECDED. |
| [Esquema elétrico em PDF](../diagramas/Trabalho%201%20-%20Arquitetura%20de%20Computadores.pdf) | Desenho das conexões (revisão 7.0, de 29/10/2026), com fonte editável em KiCad na pasta `diagramas`. |
| [Rotina em assembly](../scripts/rotina_assembly.txt) | Exemplo com acionamento abaixo de 20%, desligamento acima de 95% e alarme abaixo de 10%. |
| [Roteiro de testes](../tests/setup-testes.md) | Procedimentos e resultados esperados para os itens 4.1 a 4.10 do enunciado. |

## Entradas, saídas e montagem

O sketch converte cada leitura analógica em um percentual de 0 a 100 pela divisão inteira `(leitura × 100 + 511) / 1023`, que arredonda para o inteiro mais próximo.

Cada reservatório tem uma saída de bomba e um alarme corretivo. O buzzer usa o pino `D49` e fica ligado enquanto houver algum alarme ativo. A instrução `LED x` habilita o indicador RGB. Após uma leitura válida, o indicador fica vermelho até 20%, amarelo de 21% a 70% e verde acima de 70%.

### Leitura dos níveis

Cada potenciômetro funciona como divisor de tensão. O cursor fornece ao Arduino uma tensão que varia conforme a posição do eixo. O sketch lê essa tensão com `analogRead()` e trabalha com valores de 0 a 1023. Uma leitura bruta de 0 corresponde a 0%, uma leitura de 512 corresponde a aproximadamente 50% e uma leitura de 1023 corresponde a 100%.

A conversão acrescenta 511 antes da divisão inteira para arredondar o resultado. O percentual fica guardado no registrador de nível do reservatório e também é copiado para `ACC`. O sketch marca a leitura como válida para que a comparação possa usar esse valor. Ao receber uma nova leitura, substitui o valor do reservatório correspondente e preserva as leituras dos outros dois.

### Alarmes por reservatório

`ALARM x` ativa o alarme do reservatório indicado, enquanto `SILENCE x` desativa somente o alarme desse reservatório. Por exemplo, depois de `ALARM 1` e `ALARM 2`, o comando `SILENCE 1` mantém o buzzer ligado porque o alarme 2 continua ativo. Nesse caso, `SILENCE 2` desliga o buzzer.

### Displays de sete segmentos

O enunciado prevê um dígito para indicar a faixa de nível. O grupo usou dois displays para exibir a identificação do reservatório e o percentual. A instrução `INFO x, modo` mostra `L1`, `L2` ou `L3` por cerca de um segundo antes do nível quando `modo` é `1`; quando `modo` é `0`, atualiza diretamente o nível. Uma leitura de 100% fica registrada como 100 e aparece como `99` nos displays.

### Pinagem

| Sinal | Sketch atual | Esquema em PDF | Enunciado |
| --- | --- | --- | --- |
| Potenciômetros 1 a 3 | `A0`, `A1`, `A2` | `A0`, `A1`, `A2`, componentes de 50 kΩ | `A0`, `A1`, `A2`, componentes de 10 kΩ |
| Saídas das bombas 1 a 3 | `46`, `47`, `48` | LEDs RGB rotulados como bombas nos pares `38/39`, `40/41`, `42/43` | `30`, `31`, `32` |
| Indicação visual auxiliar | RGB nos pares `38/39`, `40/41`, `42/43` | LEDs preventivos rotulados nos pinos `46`, `47`, `48` | LEDs preventivos nos pinos `33`, `34`, `35` |
| Buzzer | `D49` | `D52` na rev. 7.0, via transistor NPN 2N2222A e resistor de base R24 de 330 Ω | `D45` |
| Display, segmentos na ordem `a–g` | Dezenas: `36, 37, 30, 35, 34, 28, 31` · Unidades: `26, 29, 25, 24, 27, 23, 22` | Primeiro display: `22–28` · Segundo display: `29–35` | Um display em `22–28`, com o dígito correspondente à faixa de nível |

O esquema da revisão 7.0 mostra displays de ânodo comum, resistores de 330 Ω e GND comum. Os cursores de POT1, POT2 e POT3 estão em `A0`, `A1` e `A2` no esquema e no sketch. A tabela também registra as diferenças de pinagem em relação ao [enunciado](https://claytonjasilva.github.io/arq_aulas/trabalho_ap1_tres_bombas.html).

### Ligações do protótipo

| Componente | Ligação e função |
| --- | --- |
| Potenciômetros | O esquema liga os extremos a `+5 V` e ao GND. Os cursores de POT1, POT2 e POT3 seguem para `A0`, `A1` e `A2`. |
| Displays | Cada segmento do esquema tem um resistor em série de 330 Ω. Como os displays são de ânodo comum, o sketch acende um segmento com nível lógico baixo. |
| LEDs RGB | O sketch usa os canais vermelho e verde. A combinação dos dois produz amarelo. A indicação só é atualizada para o reservatório com sinalização habilitada e leitura válida. |
| Buzzer | No esquema, o buzzer recebe alimentação de `+5 V` e o transistor 2N2222A comuta o retorno ao GND. O sinal de controle chega à base pelo resistor R24. Na versão documentada do projeto, esse sinal vem de `D49`. |

O grupo acrescentou um segundo display e alterou a distribuição das saídas digitais em relação ao enunciado. O PDF da revisão 7.0 registra a distribuição anterior dos displays e do buzzer. A coluna do sketch na tabela de pinagem indica o que o código publicado efetivamente configura. Essa distinção também vale para os pinos `46` a `48`, que o sketch usa como saídas de bomba e que o desenho rotula como indicadores preventivos.

## Arquitetura simulada

1. O **montador** valida o mnemônico e os operandos recebidos pelo monitor serial e codifica a instrução em uma palavra lógica de 18 bits.
2. O **controlador de memória** acrescenta seis bits de redundância SECDED e armazena a palavra protegida em `MEM_PROG`. Na busca, pode corrigir um bit invertido e regravar a palavra. A detecção de dois bits invertidos interrompe a execução.
3. A **Unidade de Controle** usa o `PC` para localizar a instrução, recebe a palavra lógica recuperada no `IR` e extrai os campos com operações sobre bits. Após executar a instrução, avança o `PC`, carrega o destino de um desvio ou encerra a execução.
4. A **ULA** faz a subtração de `CMP`. O resultado fica no acumulador assinado `ACC` (`int16_t`), e as flags `L`, `Z` e `G` indicam resultado menor, igual ou maior que zero.

A memória tem 128 posições, de `0x00` a `0x7F`. As palavras somam 288 bytes de dados lógicos ou 384 bytes com SECDED. Como cada posição usa um `uint32_t`, o vetor `MEM_PROG[128]` ocupa 512 bytes. Outro vetor registra quais posições estão ocupadas.

### Registradores e estados

| Elemento | Função na CPU simulada |
| --- | --- |
| `PC` | Guarda o endereço da próxima instrução. O início de uma execução ajusta o endereço para `0x00`. |
| `MAR` | Guarda o endereço acessado pelo controlador de memória na leitura ou na gravação. |
| `MBR` | Guarda os 18 bits da instrução que entram ou saem do controlador de memória. |
| `MBR_RAW` | Guarda a representação de 24 bits usada no acesso à memória. Após uma correção de um bit na busca, recebe a palavra corrigida. |
| `IR` | Guarda a instrução que será decodificada e executada. |
| `ACC` | Guarda o último percentual lido ou o resultado da última operação de comparação. |
| `REG` | Registrador geral assinado usado para contadores e cálculos incrementais. |
| `NIVEL` e `NIVEL_VALIDO` | Guardam a última leitura de cada reservatório e indicam se ela pode ser usada por `CMP`. |
| `FLAG_L`, `FLAG_Z`, `FLAG_G` | Guardam o resultado da comparação usado pelas instruções de desvio. |
| `FLAG_REG` | É atualizada por `CMPREG` e indica que `REG` é menor que o limite; `JREG` usa essa flag. |

### Busca e decodificação

No início de cada ciclo, a Unidade de Controle copia o endereço do contador de programa (`PC`) para o registrador de endereço de memória (`MAR`). O controlador verifica a palavra de 24 bits armazenada naquela posição e entrega os 18 bits da instrução pelo registrador `MBR`. A Unidade de Controle copia esses bits para `IR`, de onde o decodificador extrai o opcode e os operandos. Uma tentativa de executar uma posição vazia gera um erro.

### Organização da palavra e proteção da memória

O montador posiciona o opcode nos bits 17 a 14, o primeiro operando nos bits 13 a 7 e o segundo nos bits 6 a 0. A instrução `CMP 1, 30`, por exemplo, usa o primeiro operando para selecionar o reservatório e o segundo para guardar o limite. Em uma instrução de desvio, o primeiro operando guarda um endereço entre `0x00` e `0x7F`.

Na gravação, o controlador distribui os 18 bits da instrução em uma palavra de 24 bits. As posições 1, 2, 4, 8 e 16 guardam os bits de paridade de Hamming, e a posição 0 guarda a paridade global. Durante a leitura, a síndrome combina o resultado das cinco verificações de paridade para indicar a posição de uma possível falha. A paridade global ajuda a distinguir uma alteração de um bit de uma alteração de dois bits.

| Condição da palavra | Comportamento na busca da instrução |
| --- | --- |
| Nenhuma falha detectada | Extrai os 18 bits da instrução e continua a execução. |
| Um bit invertido | Corrige o bit, regrava a palavra corrigida e executa a instrução recuperada. |
| Dois bits invertidos | Sinaliza o erro, interrompe a execução e desliga as saídas de bombas e alarmes. |

O comando `MEM` mostra a palavra física de 24 bits, a instrução de 18 bits e o estado da verificação de paridade. É uma consulta. A regravação da palavra corrigida ocorre quando a CPU busca a instrução para executar.

### Conjunto de instruções

A palavra lógica tem 4 bits de opcode e dois operandos de 7 bits cada. Os espaços nos exemplos separam esses campos. Campos sem uso ficam zerados. Os opcodes `0000` a `1110` identificam instruções diretamente; o opcode `1111` é estendido e identifica as instruções do registrador geral, com a operação definida pelo campo `OP2`.

| Instrução | Efeito | Exemplo | Palavra lógica de 18 bits |
| --- | --- | --- | --- |
| `READ x` | Lê o nível e atualiza `NIVEL` e `ACC`. | `READ 1` | `0000 0000001 0000000` |
| `ON x` | Liga a saída da bomba. | `ON 1` | `0001 0000001 0000000` |
| `OFF x` | Desliga a saída da bomba. | `OFF 1` | `0010 0000001 0000000` |
| `ALARM x` | Ativa o alarme corretivo da bomba. | `ALARM 2` | `0011 0000010 0000000` |
| `LED x` | Habilita a indicação RGB. | `LED 3` | `0100 0000011 0000000` |
| `INFO x, modo` | Faz nova leitura; com `modo = 1`, mostra a identificação e depois o nível; com `modo = 0`, atualiza diretamente o nível. | `INFO 1, 1` | `0101 0000001 0000001` |
| `SILENCE x` | Desativa o alarme corretivo da bomba. | `SILENCE 2` | `0110 0000010 0000000` |
| `LEDOFF x` | Desabilita e apaga a indicação RGB. | `LEDOFF 3` | `0111 0000011 0000000` |
| `CMP x, n` | Subtrai o limite da última leitura válida e atualiza `ACC` e as flags. | `CMP 1, 30` | `1000 0000001 0011110` |
| `JMP a` | Salta para o endereço indicado. | `JMP 0x03` | `1001 0000011 0000000` |
| `JL a` | Salta se a última comparação indicou menor. | `JL 0x09` | `1010 0001001 0000000` |
| `JE a` | Salta se a última comparação indicou igual. | `JE 0x0A` | `1011 0001010 0000000` |
| `JG a` | Salta se a última comparação indicou maior. | `JG 0x0B` | `1100 0001011 0000000` |
| `WAIT t` | Aguarda `t × 100 ms` e continua atendendo o monitor serial. | `WAIT 10` | `1101 0001010 0000000` |
| `CMPREG n` | Compara o registrador geral com `n` e ativa `FLAG_REG` quando `REG < n`. | `CMPREG 20` | `1111 0001010 0000000` |
| `JREG a` | Salta para `a` quando `FLAG_REG` está ativa. | `JREG 0x05` | `1111 0000101 0000001` |
| `REGSUM n` | Soma `n` ao registrador geral; aceita valores de -64 a 63. | `REGSUM -1` | `1111 1111111 0000010` |
| `HALT` | Encerra o programa e mantém as saídas. | `HALT` | `1110 0000000 0000000` |

Com a última leitura da bomba 1 em 20%, `CMP 1, 30` grava `−10` no `ACC` e ativa a flag `L`. Em complemento de dois, `−10` corresponde a `1111111111110110` nos 16 bits do acumulador. O sketch mostra também uma representação de 18 bits com extensão de sinal no monitor.

### Validação do programa

Durante a carga, o montador verifica a quantidade de operandos, a faixa dos valores e o formato dos endereços. As instruções que recebem um número de reservatório aceitam valores de 1 a 3. O limite de `CMP` é um inteiro de 0 a 100. O comando `END` também verifica se os destinos dos desvios apontam para posições carregadas.

Durante a execução, `CMP` exige que o reservatório indicado já tenha uma leitura válida. Os desvios condicionais exigem uma comparação válida. Uma sequência comum é `READ 1`, `CMP 1, 30` e `JL 0x09`. Se a leitura for de 29%, a diferença será −1 e o desvio será executado. A 30%, a diferença será zero e a execução seguirá para o endereço seguinte.

`REGSUM` aceita imediatos assinados de -64 a 63. A combinação `REGSUM 1`,
`CMPREG 20` e `JREG a` permite repetir um ciclo até o registrador alcançar
20, sem bloquear o monitor serial. `JREG` exige um `CMPREG` anterior válido.

## Como executar

1. Abra o sketch na Arduino IDE. Se a IDE pedir uma pasta com o mesmo nome do sketch, salve `ArquiteturaSimulada.ino` dentro da pasta `ArquiteturaSimulada`.
2. Selecione a placa Arduino Mega 2560 e a porta correspondente. Após conferir a pinagem da montagem, carregue o sketch.
3. Configure o monitor serial para 115200 baud e envio de nova linha.
4. Envie os comandos abaixo, um por linha.

```text
LOAD
READ 1
ON 1
OFF 1
HALT
END
MEM 0x00 0x03
RUN
STEP
STEP
STEP
STEP
```

Durante a carga, o montador ignora linhas vazias e comentários iniciados por ponto e vírgula. A consulta `MEM 0x00 0x03` permite conferir as quatro instruções antes da execução.

### Comandos do monitor serial

| Comando | Uso |
| --- | --- |
| `LOAD` | Limpa a memória de programa, reinicia os estados e abre a carga de um novo programa. |
| `END` | Verifica as instruções carregadas e encerra a carga. |
| `RUN` | Reinicia a execução em `0x00` e habilita o modo passo a passo. |
| `STEP` ou `*` | Executa a próxima instrução no modo passo a passo. |
| `AUTO` | Reinicia em `0x00` e executa as instruções em sequência, respeitando os saltos e as esperas, sem emitir o trace detalhado de cada instrução. |
| `MEM` ou `MEM 0x00 0x03` | Consulta toda a memória ou apenas o intervalo especificado. |
| `STATUS` | Mostra o modo, os registradores, os níveis, as saídas e os contadores de falhas de memória. |
| `FLIP 0x00 10` | Inverte um bit físico para testar a proteção SECDED. |
| `STOP` | Interrompe a execução ou a espera e desliga as saídas das bombas e dos alarmes. |

`WAIT` usa intervalos de 100 ms. Assim, `WAIT 10` aguarda um segundo. Durante a espera, o monitor continua processando comandos como `STATUS` e `STOP`. Um `STEP` enviado durante a espera é rejeitado.

### Exemplo passo a passo

No programa acima, `RUN` prepara a execução a partir de `0x00`. Com o potenciômetro 1 ajustado para uma leitura de 20%, cada `STEP` produz o seguinte estado:

| Endereço executado | Instrução | Efeito esperado |
| --- | --- | --- |
| `0x00` | `READ 1` | Registra 20% no nível do reservatório 1 e coloca `20` em `ACC`. |
| `0x01` | `ON 1` | Liga a saída da bomba 1. |
| `0x02` | `OFF 1` | Desliga a saída da bomba 1. |
| `0x03` | `HALT` | Encerra o programa com a saída da bomba 1 desligada. |

`HALT` mantém o estado das saídas no momento da parada. Para interromper a execução e desligar as saídas das bombas e dos alarmes, use `STOP`.

## Testes

Os resultados abaixo foram obtidos executando o sketch em C++ com entradas analógicas e tempo simulados. Os resultados de bancada serão registrados no [roteiro de testes](../tests/setup-testes.md).

| Verificação | Resultado observado em simulação |
| --- | --- |
| Carga e execução passo a passo | Quatro instruções carregadas de `0x00` a `0x03` sem ligar a saída de bomba. A sequência de passos leu o nível, ligou a saída, desligou a saída e encerrou em `HALT`. |
| SECDED | A inversão de um bit foi corrigida durante a busca, com regravação da palavra. A inversão de dois bits antes da busca levou ao modo de erro. |
| Controle automático com os saltos corrigidos | Com as três entradas seguindo 29% → 30% → 80% → 81% → 80% → 30% → 29%, as três saídas seguiram ligada → ligada → ligada → desligada → desligada → desligada → ligada. `STOP` interrompeu a espera e desligou as saídas. |

### Verificações do roteiro

| Item | Procedimento e critério de verificação | Registro disponível |
| --- | --- | --- |
| 4.1 — Carga | Carregar quatro instruções, consultar a memória e conferir os endereços e o estado das saídas. Repetir com comentários e linhas vazias. | Carga básica verificada em simulação. |
| 4.2 — Ciclo de instrução | Executar o programa curto com `STEP` e conferir endereço, `IR`, estado da saída e avanço de `PC`. | Sequência de execução verificada em simulação. |
| 4.3 — Leitura independente | Ajustar os três potenciômetros para níveis diferentes e conferir as leituras individuais, incluindo 0% e 100%. | Resultado de bancada a registrar. |
| 4.4 — Comparação e desvios | Testar valores abaixo, iguais e acima do limite. Conferir o sinal de `ACC`, a flag correspondente e o endereço executado após o desvio. | Desvios menor e maior exercitados na simulação do teste 4.9. |
| 4.5 — Bombas e alarmes | Testar cada saída de bomba e verificar que o buzzer continua ligado se um dos alarmes permanecer ativo. | Saída da bomba exercitada em simulação. Resultado de bancada a registrar. |
| 4.6 — Displays | Executar `INFO` para os três reservatórios e conferir a identificação e o percentual exibidos, incluindo o tratamento de 100%. | Resultado de bancada a registrar. |
| 4.7 — Memória | Consultar um intervalo, preencher as 128 posições e verificar se uma instrução adicional é rejeitada. | Consulta de intervalo verificada em simulação. |
| 4.8 — Erros | Testar entradas inválidas e injetar falhas de um e dois bits. | Correção de um bit e interrupção por dois bits verificadas em simulação. |
| 4.9 — Controle automático | Variar os níveis em torno de 30% e 80%, conferir a manutenção do estado e usar `STOP` durante a espera. | Programa com os saltos corrigidos verificado em simulação. |
| 4.10 — Encerramento | Executar `HALT` e enviar outro `STEP` para conferir que a execução permanece encerrada. | Verificado em simulação. |

### Reprodução da injeção de falhas

Com o programa curto carregado e validado por `END`, envie `RUN`, `FLIP 0x00 10` e `STEP`. A busca deve recuperar a instrução `READ 1`, corrigir a palavra na memória e incrementar o contador de correções. Use `STATUS` para consultar o contador.

Para testar dois bits, parta da palavra já corrigida. Envie `RUN`, `FLIP 0x00 10`, `FLIP 0x00 11` e `STEP`. A primeira instrução deve falhar na busca, e a CPU deve entrar no modo de erro. As duas inversões precisam ocorrer antes do passo de execução, pois uma busca entre elas pode corrigir a primeira alteração.

### Correção do teste 4.9

O programa do teste lê os três reservatórios em sequência. Para cada bomba, liga a saída quando o nível fica abaixo de 30% e desliga quando passa de 80%. Entre 30% e 80%, inclusive, a saída mantém o estado anterior. Assim, a bomba pode continuar ligada a 60% após partir de 29%, ou continuar desligada a 60% depois de passar por 81%. Ao terminar as três leituras, `WAIT 10` espera um segundo e `JMP 0x03` inicia outra passagem.

A simulação do teste 4.9 usou os destinos de salto abaixo, mantendo as instruções nos endereços do roteiro. Em cada bloco, os dois `JMP` locais apontam para a mesma posição. A rotina em `scripts/rotina_assembly.txt` usa outros limites de acionamento.

| Bomba | Salto abaixo de 30% | Salto acima de 80% | Continuação do laço |
| --- | --- | --- | --- |
| 1 | `JL 0x09` | `JG 0x0B` | `JMP 0x0C` nas posições `0x08` e `0x0A` |
| 2 | `JL 0x12` | `JG 0x14` | `JMP 0x15` nas posições `0x11` e `0x13` |
| 3 | `JL 0x1B` | `JG 0x1D` | `JMP 0x1E` nas posições `0x1A` e `0x1C` |

### Programa de controle automático com os saltos corrigidos

Carregue o bloco abaixo pelo monitor serial e envie `AUTO` depois de `END`. Os primeiros três comandos assembly habilitam a indicação RGB. O salto final retorna ao primeiro `READ`, mantendo a indicação habilitada entre as passagens.

```text
LOAD
LED 1
LED 2
LED 3

READ 1
CMP 1, 30
JL 0x09
CMP 1, 80
JG 0x0B
JMP 0x0C
ON 1
JMP 0x0C
OFF 1

READ 2
CMP 2, 30
JL 0x12
CMP 2, 80
JG 0x14
JMP 0x15
ON 2
JMP 0x15
OFF 2

READ 3
CMP 3, 30
JL 0x1B
CMP 3, 80
JG 0x1D
JMP 0x1E
ON 3
JMP 0x1E
OFF 3

WAIT 10
JMP 0x03
END
```

Para verificar uma saída de cada vez na bancada, ajuste o potenciômetro do reservatório correspondente para um valor abaixo de 30%, depois para um valor intermediário e, por fim, para um valor acima de 80%. Após o desligamento, volte à faixa intermediária. A saída deve manter o estado desligado até o nível cair abaixo de 30%. Registre o nível informado pelo monitor, o estado da saída e o resultado observado.
