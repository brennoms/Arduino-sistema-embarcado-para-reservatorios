<div align="center">

# Sistema Inteligente de Reservatórios

### Monitoramento, automação e controle de três reservatórios com Arduino

<p>
  <img src="https://img.shields.io/badge/Arduino-Mega%202560-00979D?style=for-the-badge&logo=arduino&logoColor=white" alt="Arduino Mega 2560">
  <img src="https://img.shields.io/badge/linguagem-C%2B%2B-00599C?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++">
  <img src="https://img.shields.io/badge/licença-MIT-2ea44f?style=for-the-badge" alt="Licença MIT">
</p>

<p>
  Protótipo acadêmico para monitoramento e controle de três reservatórios.
</p>

</div>

---

## Sobre o projeto

Este projeto controla **três reservatórios de água** usando um
**Arduino Mega 2560**. O sistema lê os níveis, aciona as bombas e exibe o
estado dos reservatórios por meio de LEDs, displays e alarmes.

## O que o sistema faz?

- Mede o nível de cada reservatório por meio de potenciômetros;
- Converte as leituras em percentuais de 0% a 100%;
- Aciona ou desliga as bombas conforme o programa carregado;
- Exibe informações nos displays de sete segmentos;
- Indica visualmente o nível com LEDs RGB:
  - 🔴 nível crítico;
  - 🟡 nível intermediário;
  - 🟢 nível adequado;
- Emite um alerta sonoro quando existe uma condição de alarme;
- Permite carregar e executar rotinas de controle pelo monitor serial;
- Protege a memória do programa contra falhas simples de leitura.

## Funcionamento

```text
┌─────────────────────┐
│  Níveis dos tanques │  ← potenciômetros
└──────────┬──────────┘
           │ leitura
           ▼
┌─────────────────────┐
│   Arduino Mega 2560 │  ← interpreta a rotina de controle
└──────┬──────┬───────┘
       │      │
       │      └── displays, LEDs e buzzer
       └───────── bombas dos reservatórios
```

O programa lê cada reservatório, compara o nível com limites definidos e
controla as saídas correspondentes.

## Arquitetura simulada e SECDED

O Arduino Mega 2560 executa uma arquitetura de computador criada para o
projeto. A rotina em assembly é recebida pelo monitor serial, convertida em
instruções e armazenada na memória de programa. Em seguida, a unidade de
controle busca e executa cada instrução, acionando as entradas e saídas do
protótipo.

A arquitetura possui:

- **Unidade de controle:** busca, decodifica e executa as instruções;
- **Memória de programa:** armazena até 128 instruções;
- **ULA:** realiza operações e comparações dos níveis;
- **Registradores:** guardam endereços, instruções, resultados e estados;
- **Interface de entrada e saída:** conecta os sensores, bombas, LEDs,
  displays e buzzer.

As instruções utilizam uma palavra lógica de 18 bits. Para proteger a memória,
o sistema utiliza **SECDED** (*Single Error Correction, Double Error
Detection*). Essa proteção adiciona bits de verificação à instrução e permite:

- corrigir automaticamente um erro de um bit;
- detectar um erro de dois bits;
- interromper a execução quando o erro não pode ser corrigido.

Os detalhes da ISA, da memória e do SECDED estão na
[documentação da arquitetura](documentacao/documentacao_arquitetura.md).

## Componentes principais

| Componente | Função |
| --- | --- |
| Arduino Mega 2560 | Controla o protótipo e executa a arquitetura simulada |
| 3 potenciômetros | Representam os níveis dos reservatórios |
| 3 saídas de bomba | Representam o enchimento ou acionamento de cada reservatório |
| 3 LEDs RGB | Mostram visualmente a faixa de nível |
| 2 displays de 7 segmentos | Exibem a identificação e o percentual |
| Buzzer | Emite alertas sonoros |

## Organização do repositório

| Pasta/arquivo | Conteúdo |
| --- | --- |
| [`scripts/`](scripts/) | Sketch do Arduino e rotinas de controle |
| [`documentacao/`](documentacao/) | Explicações do sistema, da montagem e da arquitetura |
| [`diagramas/`](diagramas/) | Esquemas elétricos, projeto KiCad e modelo do suporte |
| [`tests/`](tests/) | Roteiro para validação do protótipo |
| [`requirements.txt`](requirements.txt) | Dependência auxiliar para comunicação serial |
| [`LICENSE`](LICENSE) | Licença MIT do projeto |

## Como executar

### 1. Abra o sketch

Abra [`scripts/ArquiteturaSimulada.ino`](scripts/ArquiteturaSimulada.ino) na
Arduino IDE e selecione a placa **Arduino Mega 2560**.

### 2. Grave o programa

Conecte a placa, escolha a porta serial correta e faça o upload do sketch.

### 3. Envie a rotina pelo Python

Instale a dependência:

```bash
python -m pip install -r requirements.txt
```

Com a placa conectada, execute:

```bash
python scripts/enviarViaSerial.py
```

O script lista as portas seriais disponíveis, envia o conteúdo de
[`scripts/rotina_assembly.txt`](scripts/rotina_assembly.txt) e finaliza com
`END`. Para informar a porta diretamente:

```bash
python scripts/enviarViaSerial.py /dev/ttyUSB0
```

No Windows, substitua a porta por algo como `COM3`. Após o envio, abra o
monitor serial da Arduino IDE e envie `RUN` ou `AUTO` para iniciar a rotina.
Para acompanhar as respostas diretamente pelo Python, use a opção
`--monitorar`:

```bash
python scripts/enviarViaSerial.py --monitorar
```

### 4. Teste o protótipo

Gire os potenciômetros para simular diferentes níveis e observe a resposta das
bombas, dos LEDs, dos displays e do buzzer. O
[roteiro de testes](tests/setup-testes.md) reúne os cenários previstos.

Consulte o [diagrama de ligações](diagramas/diagrama_ligacoes.pdf) para montar
o circuito.

## Documentação

- [Documentação do sistema de três reservatórios](documentacao/Documentacao_Sistema_de_Tres_Bombas.md)
- [Documentação da arquitetura](documentacao/documentacao_arquitetura.md)
- [Sketch principal do Arduino](scripts/ArquiteturaSimulada.ino)
- [Rotina de controle em assembly](scripts/rotina_assembly.txt)
- [Roteiro de testes](tests/setup-testes.md)
- [Esquema de ligações em PDF](diagramas/diagrama_ligacoes.pdf)
- [Projeto da placa no KiCad](diagramas/projeto%20kicad/)
- [Créditos do suporte impresso em 3D](diagramas/case%20para%20arduino/creditos.md)

## Contexto acadêmico

Projeto desenvolvido para a disciplina de **Arquitetura de Computadores do
IBMEC**, sob orientação do professor
[Clayton J. A. Silva](https://github.com/claytonjasilva).

## Integrantes

| Integrante | O que fez |
| --- | --- |
| [Brenno Marques Silva](https://github.com/brennoms) | organizou o projeto — criou, documentou e testou os algoritmos e a ISA — soldou os componentes — projetou e colou a case — fez os furos para os componentes no acrílico — documentou o projeto  |
| [Manuela Barros Menezes da Silva](https://github.com/manuubms) | diagramou as ligações — separou os componentes — auxiliou a soldar os componentes — ajudou a projetar, tirou as medidas, criou as etiquetas e envelopou a case — ajudou a fazer os furos no acrílico |
| [Enzo Araujo Zambrotti](https://github.com/eaz-dev) | modelou o protótipo do projeto com [Wokwi](https://wokwi.com/) — fez alguns testes |
| [João Gois de Otoni](https://github.com/JGois1) | fez o arquivo com os cortes necessários para montar a case |
| [Santiago Alejo Babi Andreiu](https://github.com/santiago-andreiu-alt) | documentou o projeto |



## Licença

Este projeto está disponível sob a [licença MIT](LICENSE).
