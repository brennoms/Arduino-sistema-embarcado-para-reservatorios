#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// ARQUITETURA SIMULADA — SISTEMA PROGRAMÁVEL DE TRÊS BOMBAS
//
// Palavra da ISA: 18 bits
//   bits 17..14 -> opcode (4 bits)
//   bits 13..7  -> operando 1 (7 bits)
//   bits 6..0   -> operando 2 (7 bits)
//
// Memória de programa:
//   128 posições: 0x00 .. 0x7F
//   uma palavra de 18 bits por posição
//
// A execução depende somente da palavra armazenada em MEM_PROG.
// O texto Assembly é usado somente durante a carga/montagem.
// ============================================================================

// ============================== Constantes =================================
#define BITS_OPCODE       4
#define BITS_OPERANDO     7
#define BITS_PALAVRA      18
#define TAM_MEMORIA       128
#define VALOR_MAX_OPERANDO 0x7F
#define MAX_PERCENTUAL    100

// Grupo define o limite de t. Aqui foi adotado 600 unidades de 100 ms = 60 s.
// Altere somente esta constante caso o grupo documente outro limite.
#define MAX_WAIT_TICKS    600
#define WAIT_MS            100UL

#define OPCODE_SHIFT      14
#define OPERANDO1_SHIFT    7
#define MASCARA_7_BITS    0x7FUL
#define MASCARA_PALAVRA   0x3FFFFUL

#define LINE_BUFFER_SIZE  96

// ================================ Pinagem ===================================
const uint8_t PIN_NIVEL[3] = {A0, A1, A2};
const uint8_t PIN_BOMBA[3] = {30, 31, 32};
const uint8_t PIN_LED_PREVENTIVO[3] = {33, 34, 35};
const uint8_t PIN_BUZZER = 45;
const uint8_t PIN_DISPLAY[7] = {22, 23, 24, 25, 26, 27, 28}; // a,b,c,d,e,f,g

// true  -> display comum catodo / segmentos ativos em HIGH
// false -> display comum anodo / segmentos ativos em LOW
#define DISPLAY_ACTIVE_HIGH true

// ============================= Códigos da ISA ===============================
enum Opcode : uint8_t {
    OP_READ = 0,
    OP_ON,
    OP_OFF,
    OP_ALARM,
    OP_LED,
    OP_INFO,
    OP_SILENCE,
    OP_LEDOFF,
    OP_CMP,
    OP_JMP,
    OP_JL,
    OP_JE,
    OP_JG,
    OP_WAIT,
    OP_HALT,
    OP_INVALID = 15
};

const char *NOME_OPCODE[15] = {
    "READ", "ON", "OFF", "ALARM", "LED", "INFO", "SILENCE", "LEDOFF",
    "CMP", "JMP", "JL", "JE", "JG", "WAIT", "HALT"
};

// ============================ Sinais de memória =============================
enum SinalMemoria : uint8_t {
    MEM_NENHUM = 0,
    MEM_LEITURA,
    MEM_ESCRITA
};

// ============================== Estados da UC ===============================
enum ModoExecucao : uint8_t {
    MODO_CARGA = 0,
    MODO_PARADO,
    MODO_STEP,
    MODO_AUTO,
    MODO_WAIT,
    MODO_HALT,
    MODO_ERRO
};

const char *NOME_MODO[] = {
    "CARGA", "PARADO", "STEP", "AUTO", "WAIT", "HALT", "ERRO"
};

enum FaseUC : uint8_t {
    FASE_OCIOSA = 0,
    FASE_BUSCA,
    FASE_DECODIFICA,
    FASE_EXECUTA
};

// ============================= Registradores =================================
// Registradores de comunicação da arquitetura.
uint8_t  PC = 0;                       // 7 bits efetivos; mantido como 8 para detectar limites
uint8_t  MAR = 0;                      // endereço de memória
uint32_t MBR = 0;                      // palavra completa (18 bits efetivos)
uint32_t IR = 0;                       // palavra completa (18 bits efetivos)

int16_t  ACC = 0;                      // resultado assinado; faixa útil -100..100
uint8_t  OPCODE_REG = OP_INVALID;
uint8_t  OP1 = 0;
uint8_t  OP2 = 0;

uint8_t  NIVEL[3] = {0, 0, 0};
bool     NIVEL_VALIDO[3] = {false, false, false};

bool FLAG_L = false;
bool FLAG_Z = false;
bool FLAG_G = false;
bool COMPARACAO_VALIDA = false;

// Saídas simuladas/controladas pelo Arduino.
bool BOMBA_LIGADA[3] = {false, false, false};
bool LED_PREVENTIVO[3] = {false, false, false};
bool ALARME_CORRETIVO[3] = {false, false, false};

// ================================ Memória ===================================
// Apenas 18 bits são usados.
uint32_t MEM_PROG[TAM_MEMORIA] = {0};
bool MEM_OCUPADA[TAM_MEMORIA] = {false};

// Ponteiro de carga: 0..128. 128 significa memória cheia.
uint16_t instrucao_atual = 0;
bool programaFinalizado = false;

// ============================== Controle ====================================
ModoExecucao MODO = MODO_CARGA;
FaseUC FASE = FASE_OCIOSA;
bool CLOCK = false;

// WAIT não bloqueante.
uint32_t waitTermino = 0;
ModoExecucao modoAposWait = MODO_PARADO;

// Solicitação de STEP fica separada do processamento serial.
bool stepSolicitado = false;

// Último estado do display.
uint8_t ultimoDigito = 0;

// Buffer não bloqueante do monitor serial.
char linhaSerial[LINE_BUFFER_SIZE];
uint8_t posLinhaSerial = 0;

// ============================= ULA ===========================================
enum OperacaoULA : uint8_t {
    ULA_NENHUMA = 0,
    ULA_SUBTRACAO
};

OperacaoULA ULA_OPERACAO = ULA_NENHUMA;
int16_t ULA_A = 0;
int16_t ULA_B = 0;
int16_t ULA_RESULTADO = 0;

// ============================== Utilidades ==================================
void imprimirHex2(uint8_t valor) {
    if (valor < 0x10) Serial.print('0');
    Serial.print(valor, HEX);
}

void imprimirEndereco(uint8_t endereco) {
    Serial.print(F("0x"));
    imprimirHex2(endereco);
}

void imprimirBinario18(uint32_t palavra) {
    palavra &= MASCARA_PALAVRA;
    for (int8_t bit = BITS_PALAVRA - 1; bit >= 0; --bit) {
        Serial.print((palavra >> bit) & 1UL);
    }
}

uint32_t codificarComplemento2_18(int32_t valor) {
    return ((uint32_t)valor) & MASCARA_PALAVRA;
}

void imprimirACC18() {
    Serial.print(F("ACC[18b]="));
    imprimirBinario18(codificarComplemento2_18(ACC));
}

void trimInPlace(char *texto) {
    if (texto == NULL) return;

    size_t inicio = 0;
    size_t fim = strlen(texto);

    while (inicio < fim && (texto[inicio] == ' ' || texto[inicio] == '\t' || texto[inicio] == '\r' || texto[inicio] == '\n')) {
        inicio++;
    }

    while (fim > inicio && (texto[fim - 1] == ' ' || texto[fim - 1] == '\t' || texto[fim - 1] == '\r' || texto[fim - 1] == '\n')) {
        fim--;
    }

    if (inicio > 0) {
        memmove(texto, texto + inicio, fim - inicio);
    }

    texto[fim - inicio] = '\0';
}

void removerComentario(char *texto) {
    char *comentario = strchr(texto, ';');
    if (comentario != NULL) {
        *comentario = '\0';
    }
}

void paraMaiusculas(char *texto) {
    for (size_t i = 0; texto[i] != '\0'; ++i) {
        if (texto[i] >= 'a' && texto[i] <= 'z') {
            texto[i] = texto[i] - 'a' + 'A';
        }
    }
}

bool textoVazio(const char *texto) {
    return texto == NULL || texto[0] == '\0';
}

bool parseDecimal(const char *texto, int min, int max, int *saida) {
    if (texto == NULL || texto[0] == '\0' || saida == NULL) {
        return false;
    }

    long valor = 0;
    for (size_t i = 0; texto[i] != '\0'; ++i) {
        char c = texto[i];
        if (c < '0' || c > '9') {
            return false;
        }

        valor = valor * 10L + (c - '0');
        if (valor > max) {
            return false;
        }
    }

    if (valor < min || valor > max) {
        return false;
    }

    *saida = (int)valor;
    return true;
}

int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// Endereços devem obrigatoriamente usar 0x + dois dígitos.
bool parseEnderecoHex(const char *texto, uint8_t *saida) {
    if (texto == NULL || saida == NULL) return false;
    if (strlen(texto) != 4) return false;
    if (texto[0] != '0' || (texto[1] != 'X' && texto[1] != 'x')) return false;

    int alto = hexDigit(texto[2]);
    int baixo = hexDigit(texto[3]);
    if (alto < 0 || baixo < 0) return false;

    int valor = (alto << 4) | baixo;
    if (valor < 0 || valor >= TAM_MEMORIA) return false;

    *saida = (uint8_t)valor;
    return true;
}

const char *nomeModoAtual() {
    return NOME_MODO[MODO];
}

// =============================== E/S =========================================
void atualizarBuzzer() {
    bool algumAlarme = false;
    for (uint8_t i = 0; i < 3; ++i) {
        if (ALARME_CORRETIVO[i]) {
            algumAlarme = true;
            break;
        }
    }

    digitalWrite(PIN_BUZZER, algumAlarme ? HIGH : LOW);
}

void desligarBombasEAlarmes() {
    for (uint8_t i = 0; i < 3; ++i) {
        BOMBA_LIGADA[i] = false;
        LED_PREVENTIVO[i] = false;
        ALARME_CORRETIVO[i] = false;

        digitalWrite(PIN_BOMBA[i], LOW);
        digitalWrite(PIN_LED_PREVENTIVO[i], LOW);
    }

    digitalWrite(PIN_BUZZER, LOW);
}

uint8_t faixaDisplay(uint8_t percentual) {
    if (percentual <= 10) return 1;
    if (percentual <= 20) return 2;
    if (percentual <= 30) return 3;
    if (percentual <= 40) return 4;
    if (percentual <= 50) return 5;
    if (percentual <= 60) return 6;
    if (percentual <= 70) return 7;
    if (percentual <= 90) return 8;
    return 9;
}

// Ordem: a,b,c,d,e,f,g. Padrão de display comum catodo.
const bool SEGMENTOS[10][7] = {
    {1,1,1,1,1,1,0}, // 0
    {0,1,1,0,0,0,0}, // 1
    {1,1,0,1,1,0,1}, // 2
    {1,1,1,1,0,0,1}, // 3
    {0,1,1,0,0,1,1}, // 4
    {1,0,1,1,0,1,1}, // 5
    {1,0,1,1,1,1,1}, // 6
    {1,1,1,0,0,0,0}, // 7
    {1,1,1,1,1,1,1}, // 8
    {1,1,1,1,0,1,1}  // 9
};

void apagarDisplay() {
    for (uint8_t i = 0; i < 7; ++i) {
        digitalWrite(PIN_DISPLAY[i], DISPLAY_ACTIVE_HIGH ? LOW : HIGH);
    }
    ultimoDigito = 0;
}

void mostrarDigito(uint8_t digito) {
    if (digito > 9) {
        apagarDisplay();
        return;
    }

    for (uint8_t i = 0; i < 7; ++i) {
        bool ligado = SEGMENTOS[digito][i];
        if (!DISPLAY_ACTIVE_HIGH) ligado = !ligado;
        digitalWrite(PIN_DISPLAY[i], ligado ? HIGH : LOW);
    }

    ultimoDigito = digito;
}

int lerPercentualAnalogico(uint8_t indiceBomba) {
    if (indiceBomba >= 3) return -1;

    int raw = analogRead(PIN_NIVEL[indiceBomba]);

    // Arredondamento para o inteiro mais próximo:
    // percentual = round(raw * 100 / 1023)
    int percentual = (raw * 100L + 511L) / 1023L;

    if (percentual < 0) percentual = 0;
    if (percentual > 100) percentual = 100;

    NIVEL[indiceBomba] = (uint8_t)percentual;
    NIVEL_VALIDO[indiceBomba] = true;
    ACC = (int16_t)percentual;

    return percentual;
}

void lerNivelEExibir(uint8_t indiceBomba) {
    int percentual = lerPercentualAnalogico(indiceBomba);
    if (percentual < 0) return;

    uint8_t digito = faixaDisplay((uint8_t)percentual);
    mostrarDigito(digito);

    Serial.print(F("INFO bomba "));
    Serial.print(indiceBomba + 1);
    Serial.print(F(" -> "));
    Serial.print(percentual);
    Serial.print(F("% | faixa/display="));
    Serial.println(digito);
}

// =============================== MEMORIA ====================================
bool MEMORIA(SinalMemoria sinal) {
    if (MAR >= TAM_MEMORIA) {
        return false;
    }

    switch (sinal) {
        case MEM_LEITURA:
            // Leitura sempre retorna os bits efetivamente armazenados.
            MBR = MEM_PROG[MAR] & MASCARA_PALAVRA;
            return true;

        case MEM_ESCRITA:
            MEM_PROG[MAR] = MBR & MASCARA_PALAVRA;
            MEM_OCUPADA[MAR] = true;
            return true;

        default:
            return false;
    }
}

void limparMemoria() {
    for (uint16_t i = 0; i < TAM_MEMORIA; ++i) {
        MEM_PROG[i] = 0;
        MEM_OCUPADA[i] = false;
    }
}

uint16_t contarPosicoesCarregadas() {
    uint16_t quantidade = 0;
    for (uint16_t i = 0; i < TAM_MEMORIA; ++i) {
        if (MEM_OCUPADA[i]) quantidade++;
    }
    return quantidade;
}

// ================================ Decoder ===================================
// Recebe implicitamente a palavra completa em IR e separa os campos da ISA.
Opcode DECODER() {
    IR &= MASCARA_PALAVRA;

    OPCODE_REG = (IR >> OPCODE_SHIFT) & 0x0F;
    OP1 = (IR >> OPERANDO1_SHIFT) & MASCARA_7_BITS;
    OP2 = IR & MASCARA_7_BITS;

    if (OPCODE_REG <= OP_HALT) {
        return (Opcode)OPCODE_REG;
    }

    return OP_INVALID;
}

// ================================ ULA =======================================
void ULA() {
    ULA_RESULTADO = 0;

    switch (ULA_OPERACAO) {
        case ULA_SUBTRACAO:
            ULA_RESULTADO = ULA_A - ULA_B;
            break;

        default:
            break;
    }
}

void atualizarFlagsDaULA() {
    FLAG_L = (ULA_RESULTADO < 0);
    FLAG_Z = (ULA_RESULTADO == 0);
    FLAG_G = (ULA_RESULTADO > 0);
    COMPARACAO_VALIDA = true;
}

// ============================ Validação da ISA ===============================
bool opcodeUsaBomba(Opcode opcode) {
    return opcode == OP_READ || opcode == OP_ON || opcode == OP_OFF ||
           opcode == OP_ALARM || opcode == OP_LED || opcode == OP_INFO ||
           opcode == OP_SILENCE || opcode == OP_LEDOFF;
}

bool opcodeUsaEndereco(Opcode opcode) {
    return opcode == OP_JMP || opcode == OP_JL || opcode == OP_JE || opcode == OP_JG;
}

bool opcodeUsaSoOP1(Opcode opcode) {
    return opcodeUsaBomba(opcode) || opcodeUsaEndereco(opcode) || opcode == OP_WAIT;
}

bool palavraDecodificadaValida(Opcode opcode, uint8_t op1, uint8_t op2) {
    if (opcode == OP_INVALID) return false;

    switch (opcode) {
        case OP_READ:
        case OP_ON:
        case OP_OFF:
        case OP_ALARM:
        case OP_LED:
        case OP_INFO:
        case OP_SILENCE:
        case OP_LEDOFF:
            return op1 >= 1 && op1 <= 3 && op2 == 0;

        case OP_CMP:
            return op1 >= 1 && op1 <= 3 && op2 <= MAX_PERCENTUAL;

        case OP_JMP:
        case OP_JL:
        case OP_JE:
        case OP_JG:
            return op1 < TAM_MEMORIA && op2 == 0;

        case OP_WAIT:
            return op1 <= MAX_WAIT_TICKS && op2 == 0;

        case OP_HALT:
            return op1 == 0 && op2 == 0;

        default:
            return false;
    }
}

bool validarDestinoCarregado(uint8_t endereco) {
    return endereco < TAM_MEMORIA && MEM_OCUPADA[endereco];
}

bool validarDestinosDosDesvios() {
    for (uint16_t endereco = 0; endereco < instrucao_atual; ++endereco) {
        if (!MEM_OCUPADA[endereco]) continue;

        uint32_t palavra = MEM_PROG[endereco] & MASCARA_PALAVRA;
        Opcode opcode = (Opcode)((palavra >> OPCODE_SHIFT) & 0x0F);
        uint8_t op1 = (palavra >> OPERANDO1_SHIFT) & MASCARA_7_BITS;
        uint8_t op2 = palavra & MASCARA_7_BITS;

        if (!palavraDecodificadaValida(opcode, op1, op2)) {
            Serial.print(F("ERRO no endereco "));
            imprimirEndereco((uint8_t)endereco);
            Serial.println(F(": palavra invalida para a ISA."));
            return false;
        }

        if (opcodeUsaEndereco(opcode) && !validarDestinoCarregado(op1)) {
            Serial.print(F("ERRO: destino "));
            imprimirEndereco(op1);
            Serial.print(F(" da instrucao em "));
            imprimirEndereco((uint8_t)endereco);
            Serial.println(F(" nao corresponde a uma posicao carregada."));
            return false;
        }
    }

    return true;
}

// ============================ ASSEMBLY =======================================
int ASSEMBLY(const char *instrucao) {
    if (instrucao == NULL) return -1;

    for (uint8_t i = 0; i < 15; ++i) {
        if (strcmp(instrucao, NOME_OPCODE[i]) == 0) {
            return i;
        }
    }

    return -1;
}

bool montarPalavra(const char *mnem, const char *op1Texto, const char *op2Texto, uint32_t *palavraSaida) {
    if (mnem == NULL || palavraSaida == NULL) return false;

    int opcodeInteiro = ASSEMBLY(mnem);
    if (opcodeInteiro < 0) return false;

    Opcode opcode = (Opcode)opcodeInteiro;
    uint8_t op1 = 0;
    uint8_t op2 = 0;
    int valor = 0;

    if (opcode == OP_HALT) {
        if (op1Texto != NULL || op2Texto != NULL) return false;
    }
    else if (opcode == OP_CMP) {
        if (op1Texto == NULL || op2Texto == NULL) return false;

        if (!parseDecimal(op1Texto, 1, 3, &valor)) {
            return false;
        }
        op1 = (uint8_t)valor;

        if (!parseDecimal(op2Texto, 0, MAX_PERCENTUAL, &valor)) {
            return false;
        }
        op2 = (uint8_t)valor;
    }
    else if (opcodeUsaEndereco(opcode)) {
        if (op1Texto == NULL || op2Texto != NULL) return false;

        if (!parseEnderecoHex(op1Texto, &op1)) {
            return false;
        }
    }
    else if (opcode == OP_WAIT) {
        if (op1Texto == NULL || op2Texto != NULL) return false;

        if (!parseDecimal(op1Texto, 0, MAX_WAIT_TICKS, &valor)) {
            return false;
        }
        op1 = (uint8_t)valor;
    }
    else if (opcodeUsaBomba(opcode)) {
        if (op1Texto == NULL || op2Texto != NULL) return false;

        if (!parseDecimal(op1Texto, 1, 3, &valor)) {
            return false;
        }
        op1 = (uint8_t)valor;
    }
    else {
        return false;
    }

    *palavraSaida = (((uint32_t)opcode & 0x0FUL) << OPCODE_SHIFT) |
                    (((uint32_t)op1 & MASCARA_7_BITS) << OPERANDO1_SHIFT) |
                    ((uint32_t)op2 & MASCARA_7_BITS);

    *palavraSaida &= MASCARA_PALAVRA;
    return true;
}

bool ASSEMBLER(char *instrucao) {
    if (instrucao == NULL) return false;

    // 1) Comentários não participam da montagem.
    removerComentario(instrucao);
    trimInPlace(instrucao);
    if (textoVazio(instrucao)) {
        return false;
    }

    // 2) Montagem somente durante a carga.
    if (MODO != MODO_CARGA) {
        Serial.println(F("ERRO: novas instrucoes so podem ser carregadas apos LOAD."));
        return false;
    }

    // 3) Não permitir escrita parcial ou ultrapassar 128 posições.
    if (instrucao_atual >= TAM_MEMORIA) {
        Serial.println(F("ERRO: memoria de programa cheia."));
        return false;
    }

    // 4) Tokens: espaços e vírgulas separam operandos.
    //    Assim CMP 1,30 e CMP 1, 30 resultam em dois operandos.
    char copia[LINE_BUFFER_SIZE];
    strncpy(copia, instrucao, sizeof(copia) - 1);
    copia[sizeof(copia) - 1] = '\0';

    char *token = strtok(copia, " \t,");
    if (token == NULL) return false;

    char *mnem = token;
    char *op1Texto = strtok(NULL, " \t,");
    char *op2Texto = strtok(NULL, " \t,");
    char *extra = strtok(NULL, " \t,");

    // Se houver qualquer quarto token, há operandos extras.
    if (extra != NULL) {
        Serial.println(F("ERRO: excesso de operandos."));
        return false;
    }

    uint32_t palavra = 0;

    // 5) Tudo é montado primeiro em variável temporária.
    //    A memória só é alterada depois de toda a validação.
    if (!montarPalavra(mnem, op1Texto, op2Texto, &palavra)) {
        Serial.println(F("ERRO: instrucao ou operandos invalidos."));
        return false;
    }

    // 6) Usa MAR/MBR como interface de escrita da memória.
    MAR = (uint8_t)instrucao_atual;
    MBR = palavra;

    if (!MEMORIA(MEM_ESCRITA)) {
        Serial.println(F("ERRO: falha ao gravar memoria."));
        return false;
    }

    // 7) Ponteiro avança somente após escrita válida.
    Serial.print(F("Montado em "));
    imprimirEndereco((uint8_t)instrucao_atual);
    Serial.print(F(" | "));
    imprimirBinario18(palavra);
    Serial.println();

    instrucao_atual++;
    return true;
}

// ============================== Execução ====================================
void resetarRegistradoresExecucao() {
    PC = 0;
    MAR = 0;
    MBR = 0;
    IR = 0;
    ACC = 0;
    OPCODE_REG = OP_INVALID;
    OP1 = 0;
    OP2 = 0;

    for (uint8_t i = 0; i < 3; ++i) {
        NIVEL[i] = 0;
        NIVEL_VALIDO[i] = false;
    }

    FLAG_L = false;
    FLAG_Z = false;
    FLAG_G = false;
    COMPARACAO_VALIDA = false;

    ULA_OPERACAO = ULA_NENHUMA;
    ULA_A = 0;
    ULA_B = 0;
    ULA_RESULTADO = 0;

    stepSolicitado = false;
    waitTermino = 0;
    modoAposWait = MODO_PARADO;
}

void resetarSistemaParaCarga() {
    desligarBombasEAlarmes();
    apagarDisplay();
    limparMemoria();
    resetarRegistradoresExecucao();

    instrucao_atual = 0;
    programaFinalizado = false;
    MODO = MODO_CARGA;
    FASE = FASE_OCIOSA;
    CLOCK = false;

    Serial.println(F("LOAD concluido: sistema reinicializado e memoria limpa."));
}

void prepararNovaExecucao() {
    desligarBombasEAlarmes();
    apagarDisplay();
    resetarRegistradoresExecucao();
    MODO = MODO_STEP;
    FASE = FASE_OCIOSA;
}

void erroExecucao(const char *mensagem) {
    MODO = MODO_ERRO;
    FASE = FASE_OCIOSA;
    stepSolicitado = false;

    desligarBombasEAlarmes();

    Serial.print(F("ERRO DE EXECUCAO: "));
    Serial.println(mensagem);
}

bool avancarPC() {
    if (PC >= TAM_MEMORIA - 1) {
        erroExecucao("ultima posicao atingida sem HALT ou desvio valido");
        return false;
    }

    PC++;
    return true;
}

bool validarBombaOperando(uint8_t bomba) {
    if (bomba < 1 || bomba > 3) {
        erroExecucao("identificacao de bomba invalida");
        return false;
    }
    return true;
}

void executarREAD(uint8_t bomba) {
    if (!validarBombaOperando(bomba)) return;
    if (lerPercentualAnalogico(bomba - 1) < 0) {
        erroExecucao("falha na leitura analogica");
        return;
    }

    avancarPC();
}

void executarON(uint8_t bomba) {
    if (!validarBombaOperando(bomba)) return;

    BOMBA_LIGADA[bomba - 1] = true;
    digitalWrite(PIN_BOMBA[bomba - 1], HIGH);
    avancarPC();
}

void executarOFF(uint8_t bomba) {
    if (!validarBombaOperando(bomba)) return;

    BOMBA_LIGADA[bomba - 1] = false;
    digitalWrite(PIN_BOMBA[bomba - 1], LOW);
    avancarPC();
}

void executarALARM(uint8_t bomba) {
    if (!validarBombaOperando(bomba)) return;

    ALARME_CORRETIVO[bomba - 1] = true;
    atualizarBuzzer();

    Serial.print(F("ALARM: bomba "));
    Serial.print(bomba);
    Serial.println(F(" em manutencao corretiva."));

    avancarPC();
}

void executarLED(uint8_t bomba) {
    if (!validarBombaOperando(bomba)) return;

    LED_PREVENTIVO[bomba - 1] = true;
    digitalWrite(PIN_LED_PREVENTIVO[bomba - 1], HIGH);
    avancarPC();
}

void executarINFO(uint8_t bomba) {
    if (!validarBombaOperando(bomba)) return;

    lerNivelEExibir(bomba - 1);
    avancarPC();
}

void executarSILENCE(uint8_t bomba) {
    if (!validarBombaOperando(bomba)) return;

    ALARME_CORRETIVO[bomba - 1] = false;
    atualizarBuzzer();
    avancarPC();
}

void executarLEDOFF(uint8_t bomba) {
    if (!validarBombaOperando(bomba)) return;

    LED_PREVENTIVO[bomba - 1] = false;
    digitalWrite(PIN_LED_PREVENTIVO[bomba - 1], LOW);
    avancarPC();
}

void executarCMP(uint8_t bomba, uint8_t limite) {
    if (!validarBombaOperando(bomba)) return;

    uint8_t indice = bomba - 1;

    if (!NIVEL_VALIDO[indice]) {
        erroExecucao("CMP sem leitura valida para a bomba indicada");
        return;
    }

    ULA_A = (int16_t)NIVEL[indice];
    ULA_B = (int16_t)limite;
    ULA_OPERACAO = ULA_SUBTRACAO;

    ULA();

    ACC = ULA_RESULTADO;
    atualizarFlagsDaULA();

    avancarPC();
}

void executarJMP(uint8_t destino) {
    if (!validarDestinoCarregado(destino)) {
        erroExecucao("JMP para posicao nao carregada");
        return;
    }

    PC = destino;
}

void executarCondicional(Opcode opcode, uint8_t destino) {
    if (!COMPARACAO_VALIDA) {
        erroExecucao("desvio condicional sem comparacao valida");
        return;
    }

    bool condicao = false;

    switch (opcode) {
        case OP_JL: condicao = FLAG_L; break;
        case OP_JE: condicao = FLAG_Z; break;
        case OP_JG: condicao = FLAG_G; break;
        default:
            erroExecucao("opcode condicional invalido");
            return;
    }

    if (condicao) {
        if (!validarDestinoCarregado(destino)) {
            erroExecucao("desvio condicional para posicao nao carregada");
            return;
        }
        PC = destino;
    } else {
        avancarPC();
    }
}

void executarWAIT(uint8_t ticks) {
    if (ticks > MAX_WAIT_TICKS) {
        erroExecucao("tempo de WAIT fora do limite definido pela ISA");
        return;
    }

    // O PC avanca agora para que, ao terminar a espera, a proxima instrucao seja executada.
    if (!avancarPC()) return;

    waitTermino = millis() + ((uint32_t)ticks * WAIT_MS);
    modoAposWait = MODO;
    MODO = MODO_WAIT;
}

void executarHALT() {
    MODO = MODO_HALT;
    Serial.println(F("HALT: execucao encerrada; saídas mantidas."));
}

void imprimirMnemonicoDecodificado(Opcode opcode, uint8_t op1, uint8_t op2) {
    if (opcode == OP_INVALID) {
        Serial.print(F("INVALID"));
        return;
    }

    Serial.print(NOME_OPCODE[opcode]);

    if (opcode == OP_HALT) {
        return;
    }

    Serial.print(' ');

    if (opcodeUsaEndereco(opcode)) {
        imprimirEndereco(op1);
    }
    else if (opcode == OP_CMP) {
        Serial.print(op1);
        Serial.print(F(", "));
        Serial.print(op2);
    }
    else {
        Serial.print(op1);
    }
}

void imprimirEstadosSaida() {
    Serial.print(F("BOMBAS=["));
    for (uint8_t i = 0; i < 3; ++i) {
        Serial.print(BOMBA_LIGADA[i] ? '1' : '0');
        if (i < 2) Serial.print(',');
    }
    Serial.print(F("] PREV=["));
    for (uint8_t i = 0; i < 3; ++i) {
        Serial.print(LED_PREVENTIVO[i] ? '1' : '0');
        if (i < 2) Serial.print(',');
    }
    Serial.print(F("] ALARM=["));
    for (uint8_t i = 0; i < 3; ++i) {
        Serial.print(ALARME_CORRETIVO[i] ? '1' : '0');
        if (i < 2) Serial.print(',');
    }
    Serial.println(F("]"));
}

void imprimirTrace(uint8_t enderecoExecutado, Opcode opcode, uint8_t op1, uint8_t op2, bool mostrarProximoPC) {
    Serial.println(F("--------------------------------------------------"));
    Serial.print(F("END exec=0x"));
    imprimirHex2(enderecoExecutado);
    Serial.println();

    Serial.print(F("IR="));
    imprimirBinario18(IR);
    Serial.println();

    Serial.print(F("Instrucao="));
    imprimirMnemonicoDecodificado(opcode, op1, op2);
    Serial.println();

    Serial.print(F("ACC decimal="));
    Serial.println(ACC);
    Serial.print(F(" "));
    imprimirACC18();
    Serial.println();

    Serial.print(F("FLAGS: L="));
    Serial.print(FLAG_L ? 1 : 0);
    Serial.print(F(" Z="));
    Serial.print(FLAG_Z ? 1 : 0);
    Serial.print(F(" G="));
    Serial.print(FLAG_G ? 1 : 0);
    Serial.print(F(" validCmp="));
    Serial.println(COMPARACAO_VALIDA ? 1 : 0);

    Serial.print(F("NIVEIS: ["));
    for (uint8_t i = 0; i < 3; ++i) {
        Serial.print(NIVEL[i]);
        Serial.print(NIVEL_VALIDO[i] ? '%' : '?');
        if (i < 2) Serial.print(F(", "));
    }
    Serial.println(']');

    imprimirEstadosSaida();

    if (mostrarProximoPC && MODO != MODO_HALT && MODO != MODO_ERRO) {
        Serial.print(F("Proximo PC="));
        imprimirEndereco(PC);
        Serial.println();
    } else {
        Serial.print(F("Estado="));
        Serial.println(nomeModoAtual());
    }
}

// ================================ UC ========================================
// A UC coordena BUSCA -> DECODIFICA -> EXECUTA.
// Os módulos não executam a instrução por conta própria: a UC chama cada
// componente necessário e os dados circulam pelos registradores.
void UC() {
    if (MODO != MODO_STEP && MODO != MODO_AUTO) {
        return;
    }

    if (PC >= TAM_MEMORIA) {
        erroExecucao("PC fora da memoria de programa");
        return;
    }

    if (!MEM_OCUPADA[PC]) {
        erroExecucao("tentativa de executar uma posicao de memoria nao carregada");
        return;
    }

    uint8_t enderecoExecutado = PC;

    // ------------------------------ BUSCA ------------------------------------
    FASE = FASE_BUSCA;

    MAR = PC;
    if (!MEMORIA(MEM_LEITURA)) {
        erroExecucao("falha na leitura da memoria durante a busca");
        return;
    }

    // MBR agora possui a palavra completa buscada.
    MBR &= MASCARA_PALAVRA;

    // --------------------------- DECODIFICA ----------------------------------
    FASE = FASE_DECODIFICA;

    IR = MBR;
    Opcode opcode = DECODER();

    if (!palavraDecodificadaValida(opcode, OP1, OP2)) {
        erroExecucao("opcode/campos invalidos na palavra armazenada");
        return;
    }

    // ----------------------------- EXECUTA -----------------------------------
    FASE = FASE_EXECUTA;

    switch (opcode) {
        case OP_READ:    executarREAD(OP1); break;
        case OP_ON:      executarON(OP1); break;
        case OP_OFF:     executarOFF(OP1); break;
        case OP_ALARM:   executarALARM(OP1); break;
        case OP_LED:     executarLED(OP1); break;
        case OP_INFO:    executarINFO(OP1); break;
        case OP_SILENCE: executarSILENCE(OP1); break;
        case OP_LEDOFF:  executarLEDOFF(OP1); break;
        case OP_CMP:     executarCMP(OP1, OP2); break;
        case OP_JMP:     executarJMP(OP1); break;
        case OP_JL:      executarCondicional(OP_JL, OP1); break;
        case OP_JE:      executarCondicional(OP_JE, OP1); break;
        case OP_JG:      executarCondicional(OP_JG, OP1); break;
        case OP_WAIT:    executarWAIT(OP1); break;
        case OP_HALT:    executarHALT(); break;
        default:         erroExecucao("opcode desconhecido"); return;
    }

    if (MODO == MODO_ERRO) {
        FASE = FASE_OCIOSA;
        return;
    }

    // Clock da CPU simulada: uma transicao por instrucao executada.
    CLOCK = !CLOCK;

    bool mostrarProximoPC = (MODO != MODO_HALT && MODO != MODO_ERRO);
    imprimirTrace(enderecoExecutado, opcode, OP1, OP2, mostrarProximoPC);

    FASE = FASE_OCIOSA;
}

// =========================== Monitor / comandos ==============================
void comandoMEM(char *comando) {
    uint8_t inicio = 0;
    uint8_t fim = TAM_MEMORIA - 1;

    char copia[LINE_BUFFER_SIZE];
    strncpy(copia, comando, sizeof(copia) - 1);
    copia[sizeof(copia) - 1] = '\0';

    char *token = strtok(copia, " \t"); // MEM
    token = strtok(NULL, " \t");

    if (token == NULL) {
        // MEM -> memoria completa.
    } else {
        if (!parseEnderecoHex(token, &inicio)) {
            Serial.println(F("ERRO: endereco inicial invalido. Use 0x00..0x7F."));
            return;
        }

        token = strtok(NULL, " \t");
        if (token == NULL) {
            Serial.println(F("ERRO: informe o endereco final."));
            return;
        }

        if (!parseEnderecoHex(token, &fim)) {
            Serial.println(F("ERRO: endereco final invalido. Use 0x00..0x7F."));
            return;
        }

        if (inicio > fim) {
            Serial.println(F("ERRO: intervalo invertido."));
            return;
        }

        if (strtok(NULL, " \t") != NULL) {
            Serial.println(F("ERRO: excesso de argumentos em MEM."));
            return;
        }
    }

    // Consulta não deve alterar o estado arquitetural observado pelo usuário.
    uint8_t marBackup = MAR;
    uint32_t mbrBackup = MBR;

    Serial.print(F("Posicoes carregadas: "));
    Serial.println(contarPosicoesCarregadas());
    Serial.println(F("END | PALAVRA BINARIA"));

    for (uint16_t endereco = inicio; endereco <= fim; ++endereco) {
        MAR = (uint8_t)endereco;
        MEMORIA(MEM_LEITURA);

        Serial.print(F("0x"));
        imprimirHex2((uint8_t)endereco);
        Serial.print(F(" | "));
        imprimirBinario18(MBR);

        // A coluna é somente a palavra binária; ocupação fica separada.
        Serial.println();
    }

    MAR = marBackup;
    MBR = mbrBackup;
}

void comandoSTATUS() {
    Serial.println(F("================ STATUS ================"));
    Serial.print(F("Modo: "));
    Serial.println(nomeModoAtual());

    Serial.print(F("Fase UC: "));
    switch (FASE) {
        case FASE_BUSCA: Serial.println(F("BUSCA")); break;
        case FASE_DECODIFICA: Serial.println(F("DECODIFICA")); break;
        case FASE_EXECUTA: Serial.println(F("EXECUTA")); break;
        default: Serial.println(F("OCIOSA")); break;
    }

    Serial.print(F("Clock: "));
    Serial.println(CLOCK ? 1 : 0);

    Serial.print(F("Ponteiro carga: "));
    if (instrucao_atual < TAM_MEMORIA) {
        imprimirEndereco((uint8_t)instrucao_atual);
    } else {
        Serial.print(F("128 (memoria cheia)"));
    }
    Serial.println();

    Serial.print(F("Posicoes ocupadas: "));
    Serial.println(contarPosicoesCarregadas());

    Serial.print(F("PC="));
    if (PC < TAM_MEMORIA) imprimirEndereco(PC);
    else Serial.print(F("fora-da-memoria"));
    Serial.println();

    Serial.print(F("MAR="));
    if (MAR < TAM_MEMORIA) imprimirEndereco(MAR);
    else Serial.print(F("0x"));
    if (MAR >= TAM_MEMORIA) Serial.print(MAR, HEX);
    Serial.println();

    Serial.print(F("MBR="));
    imprimirBinario18(MBR);
    Serial.println();

    Serial.print(F("IR="));
    imprimirBinario18(IR);
    Serial.println();

    Serial.print(F("OPCODE="));
    if (OPCODE_REG <= OP_HALT) {
        Serial.print(OPCODE_REG, BIN);
        Serial.print(F(" ("));
        Serial.print(NOME_OPCODE[OPCODE_REG]);
        Serial.println(')');
    } else {
        Serial.println(F("1111 (INVALIDO)"));
    }

    Serial.print(F("OP1="));
    Serial.println(OP1);
    Serial.print(F("OP2="));
    Serial.println(OP2);

    Serial.print(F("ACC="));
    Serial.println(ACC);
    Serial.print(F(" "));
    imprimirACC18();
    Serial.println();

    Serial.print(F("FLAGS: L="));
    Serial.print(FLAG_L ? 1 : 0);
    Serial.print(F(" Z="));
    Serial.print(FLAG_Z ? 1 : 0);
    Serial.print(F(" G="));
    Serial.print(FLAG_G ? 1 : 0);
    Serial.print(F(" validCmp="));
    Serial.println(COMPARACAO_VALIDA ? 1 : 0);

    for (uint8_t i = 0; i < 3; ++i) {
        Serial.print(F("Bomba "));
        Serial.print(i + 1);
        Serial.print(F(": nivel="));
        Serial.print(NIVEL[i]);
        Serial.print(F(" valido="));
        Serial.print(NIVEL_VALIDO[i] ? 1 : 0);
        Serial.print(F(" bomba="));
        Serial.print(BOMBA_LIGADA[i] ? 1 : 0);
        Serial.print(F(" LED_prev="));
        Serial.print(LED_PREVENTIVO[i] ? 1 : 0);
        Serial.print(F(" alarm="));
        Serial.println(ALARME_CORRETIVO[i] ? 1 : 0);
    }

    Serial.print(F("Buzzer="));
    bool buzzerAtivo = false;
    for (uint8_t i = 0; i < 3; ++i) {
        if (ALARME_CORRETIVO[i]) buzzerAtivo = true;
    }
    Serial.println(buzzerAtivo ? 1 : 0);

    Serial.print(F("Display="));
    if (ultimoDigito == 0) Serial.println(F("apagado"));
    else Serial.println(ultimoDigito);

    Serial.println(F("=========================================="));
}

bool programaProntoParaExecucao() {
    if (!programaFinalizado) {
        Serial.println(F("ERRO: finalize a carga com END antes de RUN/AUTO."));
        return false;
    }

    if (instrucao_atual == 0) {
        Serial.println(F("ERRO: programa vazio."));
        return false;
    }

    return true;
}

void comandoLOAD() {
    resetarSistemaParaCarga();
}

void comandoEND() {
    if (MODO != MODO_CARGA) {
        Serial.println(F("ERRO: END somente durante a carga."));
        return;
    }

    if (!validarDestinosDosDesvios()) {
        programaFinalizado = false;
        Serial.println(F("Carga nao finalizada. Corrija iniciando novo LOAD."));
        return;
    }

    programaFinalizado = true;
    resetarRegistradoresExecucao();
    desligarBombasEAlarmes();
    apagarDisplay();
    MODO = MODO_PARADO;
    FASE = FASE_OCIOSA;

    Serial.print(F("END: carga finalizada. "));
    Serial.print(instrucao_atual);
    Serial.println(F(" posicoes ocupadas; pronto para RUN ou AUTO."));
}

void comandoRUN() {
    if (!programaProntoParaExecucao()) return;

    prepararNovaExecucao();
    Serial.println(F("RUN: modo passo a passo. Use STEP ou *."));
}

void comandoAUTO() {
    if (!programaProntoParaExecucao()) return;

    prepararNovaExecucao();
    MODO = MODO_AUTO;
    Serial.println(F("AUTO: execucao continua iniciada em 0x00."));
}

void comandoSTEP() {
    if (MODO == MODO_WAIT) {
        Serial.println(F("ERRO: STEP rejeitado durante WAIT; nao ha enfileiramento."));
        return;
    }

    if (MODO != MODO_STEP) {
        Serial.println(F("ERRO: STEP disponivel somente apos RUN."));
        return;
    }

    stepSolicitado = true;
}

void comandoSTOP() {
    if (MODO == MODO_CARGA) {
        Serial.println(F("STOP: nao ha execucao para interromper durante a carga."));
        return;
    }

    desligarBombasEAlarmes();
    stepSolicitado = false;
    MODO = MODO_PARADO;
    FASE = FASE_OCIOSA;

    Serial.println(F("STOP: execucao/WAIT interrompidos; bombas e alarmes desligados."));
}

void processarLinha(char *linha) {
    if (linha == NULL) return;

    removerComentario(linha);
    trimInPlace(linha);

    if (textoVazio(linha)) {
        // Linha vazia/comentário não ocupa memória.
        return;
    }

    paraMaiusculas(linha);

    // Comandos do ambiente não são instruções da ISA e não ocupam memória.
    if (strcmp(linha, "LOAD") == 0) {
        comandoLOAD();
        return;
    }

    if (strcmp(linha, "END") == 0) {
        comandoEND();
        return;
    }

    if (strcmp(linha, "RUN") == 0) {
        comandoRUN();
        return;
    }

    if (strcmp(linha, "AUTO") == 0) {
        comandoAUTO();
        return;
    }

    if (strcmp(linha, "STEP") == 0 || strcmp(linha, "*") == 0) {
        comandoSTEP();
        return;
    }

    if (strcmp(linha, "STOP") == 0) {
        comandoSTOP();
        return;
    }

    if (strcmp(linha, "STATUS") == 0) {
        comandoSTATUS();
        return;
    }

    // MEM pode ser apenas "MEM" ou "MEM 0x00 0x0F".
    if (strncmp(linha, "MEM", 3) == 0 && (linha[3] == '\0' || linha[3] == ' ' || linha[3] == '\t')) {
        comandoMEM(linha);
        return;
    }

    // Somente durante carga uma linha Assembly pode ser montada.
    if (MODO == MODO_CARGA) {
        ASSEMBLER(linha);
        return;
    }

    Serial.println(F("ERRO: comando desconhecido ou instrucao Assembly fora da carga."));
}

void processarMonitorSerial() {
    while (Serial.available() > 0) {
        char c = (char)Serial.read();

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            linhaSerial[posLinhaSerial] = '\0';
            processarLinha(linhaSerial);
            posLinhaSerial = 0;
            linhaSerial[0] = '\0';
            continue;
        }

        if (posLinhaSerial < LINE_BUFFER_SIZE - 1) {
            linhaSerial[posLinhaSerial++] = c;
            linhaSerial[posLinhaSerial] = '\0';
        } else {
            // Descarta até Enter para não montar uma linha truncada.
            posLinhaSerial = 0;
            linhaSerial[0] = '\0';
            Serial.println(F("ERRO: linha longa demais."));
        }
    }
}

void atualizarWAIT() {
    if (MODO != MODO_WAIT) return;

    if ((int32_t)(millis() - waitTermino) >= 0) {
        MODO = modoAposWait;
        FASE = FASE_OCIOSA;

        Serial.println(F("WAIT concluido."));
    }
}

// =============================== Arduino ====================================
void setup() {
    Serial.begin(115200);

    for (uint8_t i = 0; i < 3; ++i) {
        pinMode(PIN_BOMBA[i], OUTPUT);
        pinMode(PIN_LED_PREVENTIVO[i], OUTPUT);
        pinMode(PIN_NIVEL[i], INPUT);
    }

    pinMode(PIN_BUZZER, OUTPUT);

    for (uint8_t i = 0; i < 7; ++i) {
        pinMode(PIN_DISPLAY[i], OUTPUT);
    }

    desligarBombasEAlarmes();
    apagarDisplay();
    limparMemoria();
    resetarRegistradoresExecucao();

    MODO = MODO_CARGA;
    programaFinalizado = false;

    Serial.println(F("============================================"));
    Serial.println(F("CPU SIMULADA - 18 bits / 128 palavras"));
    Serial.println(F("Digite LOAD para iniciar uma nova carga."));
    Serial.println(F("============================================"));
}

void loop() {
    // Atendimento ao monitor acontece continuamente, inclusive em AUTO/WAIT.
    processarMonitorSerial();

    // WAIT nunca bloqueia o loop: millis() determina quando liberar a próxima instrução.
    atualizarWAIT();

    // STEP executa somente quando solicitado explicitamente.
    if (MODO == MODO_STEP && stepSolicitado) {
        stepSolicitado = false;
        UC();
    }

    // AUTO executa uma instrução por passagem do loop.
    if (MODO == MODO_AUTO) {
        UC();
    }
}
