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
| [Esquema elétrico em PDF](../diagramas/diagrama_ligacoes.pdf) | Ligações da revisão 8.0, de 04/11/2026. |
| [Projeto KiCad](../diagramas/projeto%20kicad/) | Esquema e arquivos editáveis reorganizados na pasta do projeto elétrico. |
| [Modelo no Wokwi](../diagramas/Diagrama%20Arduino%20Wokwi.jpg) | Imagem do circuito organizado em ambiente de simulação. |
| [Plano de corte da caixa](../diagramas/Plano%20de%20corte.dxf) | Contornos das seis peças usadas como referência para o gabinete. |
| [Suporte do Arduino](../diagramas/case%20para%20arduino/arduino.stl) | Modelo destinado à impressão 3D do suporte interno da placa. |
| [Créditos do suporte](../diagramas/case%20para%20arduino/creditos.md) | Autoria e licença do modelo utilizado. |
| [Fotos do protótipo](../assets/) | Cinco vistas da caixa e do painel montados. |
| [Envio pela serial](../scripts/enviarViaSerial.py) | Ferramenta Python para enviar a rotina e registrar as respostas do Arduino. |
| [Dependência Python](../requirements.txt) | Versão do pyserial utilizada pelo script de comunicação. |
| [Rotina em assembly](../scripts/rotina_assembly.txt) | Programa de 75 instruções, com controle dos três reservatórios e alternância do display por um contador em `REG`. |
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

O enunciado prevê um dígito para indicar a faixa de nível. O grupo usou dois displays para exibir a identificação do reservatório e o percentual. A instrução `INFO x, modo` mostra `r1`, `r2` ou `r3` por cerca de um segundo antes do nível quando `modo` é `1`; quando `modo` é `0`, atualiza diretamente o nível. Uma leitura de 100% fica registrada como 100 e aparece como `99` nos displays.

### Pinagem

O sketch mantém as entradas analógicas em `A0`, `A1` e `A2`. Os demais sinais estão distribuídos conforme a tabela. Os pinos dos segmentos aparecem na ordem `a, b, c, d, e, f, g`.

| Sinal | Pinos do Arduino Mega 2560 | Correspondência no esquema rev. 8.0 |
| --- | --- | --- |
| Potenciômetros 1, 2 e 3 | `A0`, `A1`, `A2` | Cursores de POT1, POT2 e POT3. |
| Saídas das bombas 1, 2 e 3 | `D46`, `D47`, `D48` | LEDs identificados como Bomba 1, Bomba 2 e Bomba 3. |
| RGB do reservatório 1 | Vermelho em `D38`, verde em `D39` | RGB1. |
| RGB do reservatório 2 | Vermelho em `D40`, verde em `D41` | RGB2. |
| RGB do reservatório 3 | Vermelho em `D42`, verde em `D43` | RGB3. |
| Buzzer | `D49` | Sinal `49_BZ1`, resistor R24 e transistor 2N2222A. |
| Display de dezenas | `D36, D37, D30, D35, D34, D28, D31` | Segmentos do DISP1. |
| Display de unidades | `D26, D29, D25, D24, D27, D23, D22` | Segmentos do DISP2. |

A revisão 8.0 substitui a distribuição da revisão 7.0. O esquema passou a indicar o buzzer em `D49`, reorganizou os segmentos dos dois displays e identificou os RGBs como indicadores dos reservatórios e os LEDs individuais como saídas das bombas. Os extremos dos potenciômetros continuam entre `+5 V` e GND, mas a representação das duas alimentações foi invertida em relação ao desenho anterior. Os cursores permanecem nas mesmas entradas analógicas.

O esquema utiliza potenciômetros de 50 kΩ, displays HS-5101BS de ânodo comum e resistores de 330 Ω nos LEDs e segmentos. O sinal do buzzer chega à base do transistor por R24. O transistor comuta o retorno do buzzer ao GND e a alimentação do buzzer vem de `+5 V`. As saídas de segmentos em nível baixo acendem os respectivos segmentos.

O projeto usa dois displays e uma distribuição de pinos diferente da referência inicial do [enunciado](https://claytonjasilva.github.io/arq_aulas/trabalho_ap1_tres_bombas.html). A tabela acima segue o sketch e o esquema rev. 8.0. A seção de pinagem do arquivo de arquitetura e alguns trechos do roteiro de testes ainda conservam referências ao desenho anterior, como o buzzer em `D52` e os segmentos em sequência.

## Caixa e montagem do painel

A caixa reúne os componentes de operação e indicação em um gabinete com painel frontal inclinado. Os potenciômetros, os dois displays, o buzzer e os indicadores das três bombas ficam acessíveis pela frente. O corpo acomoda o Arduino e as ligações, enquanto uma abertura lateral permite alcançar o conector USB e a entrada de alimentação da placa.

O gabinete foi produzido com peças de acrílico, coladas e perfuradas para a instalação dos componentes. O acabamento externo inclui revestimento, etiquetas de identificação, guia de operação e um código QR na parte superior. As fotografias mostram parafusos nos painéis e pés na base. O painel inclinado deixa os controles voltados para quem opera o protótipo.

### Peças e plano de corte

O arquivo DXF usa milímetros e contém seis contornos de peças. As dimensões abaixo foram lidas dos rótulos e dos vértices do desenho.

| Peça | Quantidade | Dimensões do contorno no DXF |
| --- | --- | --- |
| Inferior | 1 | Retângulo de 130 × 192 mm. |
| Frontal | 1 | Retângulo de 130 × 160 mm. |
| Traseira | 1 | Retângulo de 130 × 115 mm. |
| Superior | 1 | Retângulo de 130 × 80 mm. |
| Lateral | 2 | Contorno com base de 192 mm, topo horizontal de 80 mm, altura de 115 mm e trecho inclinado até a base frontal. |

Essas medidas descrevem as peças planas. O arquivo apresenta os contornos externos, e as fotografias mostram os furos e recortes feitos para a instalação dos componentes. A espessura das chapas e as dimensões externas após a colagem dependem da montagem física.

### Organização dos controles

| Região do gabinete | Elementos e uso |
| --- | --- |
| Faixa esquerda do painel | Três potenciômetros identificados como `r1`, `r2` e `r3`, associados aos reservatórios. |
| Parte superior do painel | Buzzer e janela dos dois displays com a identificação `NÍVEL (%)`. |
| Matriz central | Colunas `1`, `2` e `3`, uma para cada reservatório. |
| Linha `ST` | Indicadores RGB do estado de nível. |
| Linha `B` | LEDs vermelhos que representam as saídas das bombas. |
| Lateral com guia | Instruções de uso dos potenciômetros, display, buzzer e matriz de LEDs. |
| Lateral de conexão | Abertura identificada como entrada do Arduino Mega 2560. |
| Traseira | Título do trabalho, identificação da disciplina, professor e integrantes. |
| Parte superior | Código QR e elementos de fixação da tampa. |

O guia impresso informa as cores do nível, os limites de acionamento das bombas e o limiar de alarme. A função de cada componente deve ser lida junto ao programa assembly carregado, pois os comandos de controle vêm da memória de programa.

### Suporte interno e autoria da montagem

O repositório inclui um arquivo STL para o suporte do Arduino. O arquivo de créditos identifica o modelo Arduino Holder, de Ivan_N, disponibilizado sob CC BY 4.0, e registra seu uso para impressão 3D sem alteração do modelo original. O suporte da placa é uma peça distinta do gabinete de acrílico. [Créditos do suporte](../diagramas/case%20para%20arduino/creditos.md)

O README registra as participações na construção. Brenno projetou e colou a caixa, soldou componentes e fez furos no acrílico. Manuela auxiliou no projeto, nas medidas, na soldagem e nos furos, criou as etiquetas e fez o revestimento. João preparou o arquivo dos cortes. Enzo modelou o circuito no Wokwi, e Santiago elaborou a documentação do projeto. [Participações registradas](../README.md)

### Registro fotográfico

As cinco fotografias da pasta `assets` mostram a montagem externa. Elas permitem identificar a posição dos componentes, o acabamento e o acesso às conexões. Os estados de leitura e acionamento devem ser comprovados pelas execuções e pelos registros de teste.

**Figura 1 — Vista superior e frontal.** Mostra a tampa com código QR, o painel inclinado, os potenciômetros e a matriz de indicadores.

<img src="../assets/20261112%20%281%29.jpg" alt="Vista superior e frontal da caixa com painel inclinado e potenciômetros" width="440">

**Figura 2 — Painel de operação.** Mostra a disposição dos controles `r1`, `r2` e `r3`, a janela do display, o buzzer e as linhas `ST` e `B`, divididas nas colunas dos três reservatórios.

<img src="../assets/20261112%20%282%29.jpg" alt="Painel frontal com display, buzzer e indicadores dos três reservatórios" width="440">

**Figura 3 — Guia lateral.** Mostra as instruções impressas de ajuste dos níveis, leitura do display, cores dos RGBs e estados das bombas.

<img src="../assets/20261112%20%283%29.jpg" alt="Lateral da caixa com guia de operação e limiares de controle" width="440">

**Figura 4 — Identificação do trabalho.** Mostra a etiqueta traseira com o nome do sistema, o contexto acadêmico e os integrantes, além da tampa superior.

<img src="../assets/20261112%20%284%29.jpg" alt="Parte traseira da caixa com identificação do trabalho e integrantes" width="440">

**Figura 5 — Acesso ao Arduino.** Mostra a abertura lateral que dá acesso ao conector USB e à entrada de alimentação sem retirar o painel frontal.

<img src="../assets/20261112%20%285%29.jpg" alt="Abertura lateral para acesso às conexões do Arduino Mega 2560" width="440">

### Modelo de ligações no Wokwi

A imagem do Wokwi registra a organização do Arduino Mega, dos potenciômetros, da protoboard e das saídas na etapa de modelagem. O esquema elétrico rev. 8.0 e as constantes do sketch especificam a ligação adotada na versão atual.

<img src="../diagramas/Diagrama%20Arduino%20Wokwi.jpg" alt="Modelo de ligações do circuito no Wokwi com Arduino Mega e três potenciômetros" width="760">

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
| `COMPARACAO_REG_VALIDA` | Indica que houve uma comparação de registrador antes de executar `JREG`. |

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
| `CMPREG n` | Compara o registrador geral com `n` e ativa `FLAG_REG` quando `REG < n`. | `CMPREG 20` | `1111 0010100 0000000` |
| `JREG a` | Salta para `a` quando `FLAG_REG` está ativa. | `JREG 0x05` | `1111 0000101 0000001` |
| `REGSUM n` | Soma `n` ao registrador geral; aceita valores de -64 a 63. | `REGSUM -1` | `1111 1111111 0000010` |
| `HALT` | Encerra o programa e mantém as saídas. | `HALT` | `1110 0000000 0000000` |

Com a última leitura da bomba 1 em 20%, `CMP 1, 30` grava `−10` no `ACC` e ativa a flag `L`. Em complemento de dois, `−10` corresponde a `1111111111110110` nos 16 bits do acumulador. O sketch mostra também uma representação de 18 bits com extensão de sinal no monitor.

### Instruções do registrador geral

O registrador `REG` é um inteiro assinado de 16 bits. O início de uma execução zera seu conteúdo e reinicia a validade da comparação de registrador. O operando imediato de `REGSUM` e `CMPREG` usa sete bits em complemento de dois, com valores de −64 a 63. Essa faixa limita cada operando da instrução. O registrador pode acumular várias somas ao longo do programa.

As três instruções usam o opcode `1111`. O campo `OP2` seleciona a operação e `OP1` guarda o imediato ou o endereço.

| Instrução | OP1 | OP2 | Efeito |
| --- | --- | --- | --- |
| `CMPREG n` | Imediato assinado de sete bits | `0000000` | Atualiza `FLAG_REG` com o resultado de `REG < n` e marca a comparação como válida. |
| `JREG a` | Endereço de `0x00` a `0x7F` | `0000001` | Desvia se a comparação de registrador estiver válida e `FLAG_REG` estiver ativa. |
| `REGSUM n` | Imediato assinado de sete bits | `0000010` | Soma o imediato a `REG` e avança a execução. |

Por exemplo, `REGSUM -60` codifica o imediato como `1000100`, pois 128 − 60 = 68. Já `CMPREG 20` usa `0010100` para o valor 20. O valor de `FLAG_REG` permanece associado ao último `CMPREG`; uma soma posterior exige uma nova comparação para atualizar a decisão.

A comparação de `REG` tem controle próprio. `CMPREG` preserva `ACC` e as flags `L`, `Z` e `G`, que continuam associadas a `CMP`. Os novos campos aparecem no trace e no comando `STATUS`. Combinações de `1111` com suboperações diferentes de 0, 1 ou 2 são rejeitadas.

### Modo de exibição da instrução INFO

A sintaxe atual exige dois operandos, como `INFO 2, 1` ou `INFO 2, 0`. Ambos os modos fazem uma nova leitura do reservatório, atualizam `NIVEL`, sua validade e `ACC`, e atualizam a cor do RGB quando habilitado.

Com o segundo operando igual a 1, o sketch mostra `r2` por um segundo antes de exibir o percentual. Com valor 0, atualiza diretamente o percentual. A identificação passou de `L1/L2/L3` para `r1/r2/r3`, de acordo com as etiquetas do painel. Os comentários da rotina ainda usam a letra L em algumas descrições, mas os segmentos comandados pelo sketch formam a identificação com r.

O modo com identificação usa `delay(1000)`. Durante esse intervalo, o sketch aguarda o retorno da função antes de continuar processando a serial. A temporização de `WAIT` usa outro mecanismo, baseado em `millis()`, que mantém o atendimento ao monitor durante a espera.

### Persistência do programa na EEPROM

A nova versão salva o programa na EEPROM após cada instrução montada com sucesso. A memória de execução continua sendo o vetor `MEM_PROG` em RAM. A EEPROM guarda uma cópia para restaurar o programa após reinicialização, inclusive quando a abertura de uma porta serial provoca reset pelo sinal DTR.

| Região usada na EEPROM | Conteúdo |
| --- | --- |
| Bytes 0 e 1 | Identificador `0xA55A` da gravação. |
| Bytes 2 e 3 | Quantidade de instruções armazenadas. |
| A partir do byte 4 | Palavras protegidas de `MEM_PROG`, com quatro bytes de armazenamento por posição. |

Para 128 instruções, essa estrutura usa 516 bytes, sendo quatro de cabeçalho e 512 de palavras. A gravação invalida o identificador no início, atualiza a quantidade e as palavras e grava novamente o identificador ao final. A rotina usa `EEPROM.update()` na escrita.

Na inicialização, o sketch verifica o identificador e se a quantidade está entre 1 e 128. Se a restauração for aceita, recupera as palavras, marca as posições como carregadas e deixa o sistema em `PARADO`. O operador inicia a rotina com `RUN` ou `AUTO`. Os registradores e as saídas começam reinicializados.

`LOAD` limpa a memória volátil e invalida o cabeçalho persistido. Como o salvamento acontece durante a carga, o cabeçalho também pode existir antes de `END`. A restauração verifica a assinatura e a quantidade; a validação completa dos destinos de salto é feita em `END` durante uma carga normal. O programa usado na demonstração deve ser enviado até esse comando.

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
| `LOAD` | Limpa a memória de programa, invalida a cópia persistida, reinicia os estados e abre uma nova carga. |
| `END` | Verifica as instruções carregadas e encerra a carga. |
| `RUN` | Reinicia a execução em `0x00` e habilita o modo passo a passo. |
| `STEP` ou `*` | Executa a próxima instrução no modo passo a passo. |
| `AUTO` | Reinicia em `0x00` e executa as instruções em sequência, respeitando os saltos e as esperas, sem emitir o trace detalhado de cada instrução. |
| `MEM` ou `MEM 0x00 0x03` | Consulta toda a memória ou apenas o intervalo especificado. |
| `STATUS` | Mostra o modo, os registradores, os níveis, as saídas e os contadores de falhas de memória. |
| `FLIP 0x00 10` | Inverte um bit físico para testar a proteção SECDED. |
| `STOP` | Interrompe a execução ou a espera e desliga as saídas das bombas e dos alarmes. |

`WAIT` usa intervalos de 100 ms. Assim, `WAIT 10` aguarda um segundo. Durante a espera, o monitor continua processando comandos como `STATUS` e `STOP`. Um `STEP` enviado durante a espera é rejeitado.

### Envio automático pelo Python

O script `enviarViaSerial.py` lê o arquivo `rotina_assembly.txt` localizado na mesma pasta. Ele remove comentários e linhas vazias, acrescenta `END` e envia uma linha de cada vez a 115200 baud. A rotina atual contém `LOAD` no início e deixa a execução para o operador após o envio.

Na raiz do repositório, instale a dependência e execute o script.

```bash
python -m pip install -r requirements.txt
python scripts/enviarViaSerial.py
```

Sem argumento de porta, o programa lista os dispositivos seriais e permite escolher pelo número. Também é possível informar a porta diretamente.

```bash
python scripts/enviarViaSerial.py COM3
```

No Linux, o nome da porta depende do dispositivo reconhecido pelo sistema, como `/dev/ttyACM0`. O argumento deve corresponder à placa conectada. A opção `--intervalo` altera o tempo entre linhas, cujo valor padrão é 0,05 segundo.

Por padrão, o script libera a porta após enviar a carga. Em seguida, o monitor serial da Arduino IDE pode ser aberto para receber `RUN` ou `AUTO`. A opção abaixo mantém a porta aberta para acompanhar as respostas no terminal Python.

```bash
python scripts/enviarViaSerial.py COM3 --monitorar
```

As respostas recebidas são acrescentadas ao arquivo `scripts/log.txt`. O script aguarda dois segundos após abrir a conexão, envia as linhas e registra o texto retornado. Em modo de monitoramento, continua lendo até Ctrl+C. Esse modo acompanha as respostas da placa; o envio de comandos interativos é feito pelo monitor serial depois que a porta é liberada. O arquivo de dependências fixa `pyserial==3.5`.

### Saída do montador e acompanhamento da execução

A confirmação de montagem passou a mostrar o mnemônico e os operandos junto ao endereço, à palavra lógica de 18 bits e à palavra SECDED de 24 bits. Isso permite relacionar diretamente a linha enviada ao conteúdo armazenado.

No modo passo a passo, cada instrução mantém o trace com endereço executado, `IR`, `ACC`, flags, níveis e saídas. Os campos `REG`, `FLAG_REG` e validade da comparação de registrador foram acrescentados a essa consulta. No modo `AUTO`, o trace detalhado por instrução foi suprimido. As mensagens específicas das operações, dos comandos e dos erros continuam disponíveis, e `STATUS` pode ser usado para inspecionar o estado.

### Exemplo passo a passo

No programa acima, `RUN` prepara a execução a partir de `0x00`. Com o potenciômetro 1 ajustado para uma leitura de 20%, cada `STEP` produz o seguinte estado:

| Endereço executado | Instrução | Efeito esperado |
| --- | --- | --- |
| `0x00` | `READ 1` | Registra 20% no nível do reservatório 1 e coloca `20` em `ACC`. |
| `0x01` | `ON 1` | Liga a saída da bomba 1. |
| `0x02` | `OFF 1` | Desliga a saída da bomba 1. |
| `0x03` | `HALT` | Encerra o programa com a saída da bomba 1 desligada. |

`HALT` mantém o estado das saídas no momento da parada. Para interromper a execução e desligar as saídas das bombas e dos alarmes, use `STOP`.

## Rotina de operação do protótipo

A rotina de `scripts/rotina_assembly.txt` foi reorganizada para ler os três reservatórios em cada passagem e alternar o reservatório mostrado no display. Ela contém 75 instruções, de `0x00` a `0x4A`. As decisões de bomba e alarme continuam escritas em assembly. O registrador geral controla a permanência de cada reservatório no display.

### Limiares de operação

| Função | Condição na rotina |
| --- | --- |
| Ligar bomba | Nível menor que 20%. |
| Desligar bomba | Nível maior que 95%. |
| Manter estado da bomba | Nível de 20% a 95%, inclusive. |
| Entrar no trecho de alarme | Nível menor que 10%. |
| Cor vermelha do indicador | Nível até 20%. |
| Cor amarela do indicador | Nível de 21% a 70%. |
| Cor verde do indicador | Nível acima de 70%. |

Esses limites correspondem à rotina de operação e ao guia lateral da caixa. O teste 4.9 usa os limites de 30% e 80% definidos para aquele cenário de avaliação e permanece documentado em seção própria.

### Organização dos endereços

| Trecho | Função |
| --- | --- |
| `0x00` a `0x04` | Habilita os três RGBs, inicia `REG` em −60 e exibe a identificação do reservatório 1. |
| `0x05` a `0x0C` | Lê o reservatório 1, compara os limites e encaminha os desvios de bomba e alarme. |
| `0x0D` a `0x14` | Repete o controle para o reservatório 2. |
| `0x15` a `0x1C` | Repete o controle para o reservatório 3. |
| `0x1D` a `0x2C` | Executa `WAIT 1`, incrementa o contador e escolhe qual reservatório atualizar no display. |
| `0x2D` a `0x38` | Atualiza o percentual ou apresenta a identificação na troca de reservatório. |
| `0x39` a `0x44` | Executa os blocos de ligar e desligar bombas e retorna ao fluxo de comparação. |
| `0x45` a `0x4A` | Executa os blocos de acionamento de alarme e retorna ao fluxo principal. |

O programa inicia com `REGSUM -60`. Na passagem pelo controle de display, soma 1 ao contador e testa as faixas com `CMPREG` e `JREG`. Enquanto o valor permanece abaixo de −30, atualiza o reservatório 1. Na entrada em −30, mostra `r2`; ao chegar a 0, mostra `r3`. Ao alcançar 30, faz as somas −30 e −60 para voltar a −60 e reiniciar a identificação do reservatório 1.

O comentário da rotina prevê aproximadamente quatro segundos por reservatório. Esse tempo combina cerca de 30 passagens por `WAIT 1` com a apresentação da identificação. O tempo efetivo também depende das instruções e das mensagens executadas em cada passagem.

### Comportamento do alarme na rotina atual

O acionamento do alarme e seu desligamento têm um detalhe de fluxo. Para o reservatório 1, o trecho em `0x45` executa `ALARM 1` e o salto seguinte retorna a `0x0C`, onde há `SILENCE 1`. Os reservatórios 2 e 3 seguem a mesma estrutura, retornando às respectivas instruções de silêncio.

Assim, uma passagem abaixo de 10% produz um acionamento breve seguido do desligamento do alarme daquele reservatório. A legenda lateral descreve o limiar de acionamento; a duração do som depende dessa sequência. Os comandos `ALARM` e `SILENCE` continuam mantendo estados independentes por reservatório.

## Testes

Os resultados abaixo distinguem as verificações em C++ com entradas, tempo e EEPROM simulados do registro visual da caixa. Na revisão do projeto atualizado, foram repetidos os testes básicos e exercitadas as novas instruções, os modos de INFO, a restauração do programa e a carga da rotina de 75 instruções. A execução foi feita em ambiente de desenvolvimento com as funções de hardware substituídas por simulações. Os registros de medição no protótipo pertencem ao [roteiro de testes](../tests/setup-testes.md).

| Verificação | Resultado observado em simulação |
| --- | --- |
| Carga e execução passo a passo | Quatro instruções carregadas de `0x00` a `0x03` sem ligar a saída de bomba. A sequência de passos leu o nível, ligou a saída, desligou a saída e encerrou em `HALT`. |
| SECDED | A inversão de um bit foi corrigida durante a busca, com regravação da palavra. A inversão de dois bits antes da busca levou ao modo de erro. |
| Controle automático com os saltos corrigidos | Com as três entradas seguindo 29% → 30% → 80% → 81% → 80% → 30% → 29%, as três saídas seguiram ligada → ligada → ligada → desligada → desligada → desligada → ligada. `STOP` interrompeu a espera e desligou as saídas. |

### Verificações das alterações

| Alteração | Verificação realizada | Resultado |
| --- | --- | --- |
| Extensão da ISA | Executar `REGSUM -2`, `REGSUM 1`, `CMPREG 0` e um `JREG` para um bloco carregado. | O registrador chegou a −1 e o desvio ocorreu com `FLAG_REG` ativa. |
| Faixa assinada | Tentar montar `REGSUM 64` e `CMPREG -65`. | As duas entradas foram rejeitadas sem ocupar memória. |
| Modo de INFO | Executar `INFO 1, 1` e `INFO 1, 0` com leitura simulada de 42%. | Ambos atualizaram o nível. O primeiro modo acrescentou um segundo ao tempo simulado, e o segundo atualizou diretamente. |
| Sintaxe de INFO | Enviar `INFO 1` sem o segundo operando. | O montador rejeitou a linha. |
| Persistência | Carregar um programa, executar a inicialização novamente e consultar os estados. | A memória foi restaurada com três instruções e o sistema ficou em `PARADO`, com registradores e saídas reinicializados. |
| Limpeza da EEPROM | Enviar `LOAD` após a restauração. | O cabeçalho persistido foi invalidado. |
| Rotina atual | Montar as 75 instruções e finalizar com `END`. | A quantidade e os destinos de salto foram aceitos. |
| Modo automático | Executar uma instrução em `AUTO` e examinar a saída serial simulada. | O trace detalhado por instrução ficou suprimido. |
| Caixa | Inspecionar as cinco fotografias. | Foram identificados painel, controles, etiquetas, abertura de conexão e gabinete montado. |

A restauração simulada testa a lógica do sketch. As fotografias registram a construção externa. O roteiro original ainda contém procedimentos de memória e pinagem herdados da versão anterior. Como `1111` passou a identificar uma extensão válida, o cenário de palavra inválida deve usar uma suboperação não reconhecida, como `OP2 = 3`, ou campos incompatíveis com a instrução, mantendo um código SECDED válido para testar especificamente a validação da ISA.

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


## Alterações em relação à versão anterior

A comparação considera o primeiro arquivo compactado do projeto e a versão atual, que incorpora as alterações registradas até 14/11/2026. Os ajustes já introduzidos pelo grupo na documentação do código foram incorporados ao texto deste relatório.

| Área | Alteração documentada |
| --- | --- |
| Gabinete | Inclusão da caixa de acrílico, do painel inclinado, das etiquetas, do guia lateral e do acesso ao Arduino. |
| Imagens | Inclusão de cinco fotografias em `assets` e do modelo de ligações no Wokwi. |
| Projeto mecânico | Acréscimo do DXF com seis peças, do STL do suporte e do arquivo de créditos. |
| Esquema elétrico | Publicação de `diagrama_ligacoes.pdf`, revisão 8.0, e reorganização dos arquivos editáveis em `diagramas/projeto kicad`. |
| Pinagem | Alinhamento do esquema com `D49`, com os segmentos dos displays e com as funções dos LEDs. |
| ISA | Acréscimo de `REG`, `FLAG_REG`, `CMPREG`, `JREG` e `REGSUM` pelo opcode estendido `1111`. |
| Display | Adoção de `INFO x, modo` e da identificação `r1/r2/r3`. |
| Memória persistente | Inclusão de cópia do programa na EEPROM, restauração na inicialização e invalidação por `LOAD`. |
| Monitor serial | Confirmação da linha assembly na montagem e redução do trace no modo `AUTO`. |
| Automação do envio | Inclusão de `enviarViaSerial.py`, seleção de porta, intervalo entre linhas, opção de monitoramento e log. |
| Rotina de controle | Reorganização do programa em 75 instruções com contador e alternância do reservatório no display. |
| Documentação da arquitetura | Inclusão das instruções de registrador, dos modos de INFO e da persistência. |
| Roteiro de testes | Atualização de INFO e da identificação do reservatório no teste de display. |
| Organização | Ampliação do README com funcionamento, componentes, execução, estrutura, créditos e participações. Inclusão de `requirements.txt` e de regras em `.gitignore` para `venv/`, `__pycache__/` e `log.txt`. |

A revisão também corrige a palavra binária de `CMPREG 20` para `1111 0010100 0000000`, atualiza os links do esquema elétrico e registra o fluxo de alarme presente na rotina atual. Os exemplos e as tabelas deste arquivo devem ser lidos a partir da pasta `documentacao`, onde os caminhos relativos apontam para os demais materiais do repositório.