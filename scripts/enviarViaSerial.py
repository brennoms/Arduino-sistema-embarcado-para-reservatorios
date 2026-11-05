#!/usr/bin/env python3
"""Envia a rotina Assembly do reservatorio para o Arduino."""

from __future__ import annotations

import argparse
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print(
        "Dependencia ausente: instale pyserial com "
        "'python -m pip install pyserial'.",
        file=sys.stderr,
    )
    raise SystemExit(1)


BAUD_RATE = 115200
ROTINA_ASSEMBLY = """\
; ligar a bomba em 20%
; desligar a bomba em 95%
; tocar alarme em 10%
;
; O registrador REG conta os ciclos de um segundo.
; A cada ciclo as verificacoes sao executadas novamente.
; REG=0..19 exibe INFO 1, REG=20..39 exibe INFO 2,
; REG=40..59 exibe INFO 3 e depois o contador reinicia.

LOAD

LED 1
LED 2
LED 3

INFO 1
JMP 0x05

WAIT 10
READ 1
CMP 1, 20
JL 0x2C
CMP 1, 95
JG 0x2E
CMP 1, 10
JL 0x30
SILENCE 1

READ 2
CMP 2, 20
JL 0x32
CMP 2, 95
JG 0x34
CMP 2, 10
JL 0x36
SILENCE 2

READ 3
CMP 3, 20
JL 0x38
CMP 3, 95
JG 0x3A
CMP 3, 10
JL 0x3C
SILENCE 3

REGSUM 1
CMPREG 20
JREG 0x05
CMPREG 40
JREG 0x27
CMPREG 60
JREG 0x2A
REGSUM -60
JMP 0x03

INFO 2
JMP 0x05
INFO 3
REGSUM -40
JMP 0x03

ON 1
JMP 0x09
OFF 1
JMP 0x0B
ALARM 1
JMP 0x0E

ON 2
JMP 0x11
OFF 2
JMP 0x13
ALARM 2
JMP 0x16

ON 3
JMP 0x19
OFF 3
JMP 0x1B
ALARM 3
JMP 0x1E
"""


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Envia um programa Assembly para o Arduino via serial."
    )
    parser.add_argument(
        "porta",
        nargs="?",
        help="porta serial; se omitida, sera escolhida em uma lista",
    )
    parser.add_argument(
        "--intervalo",
        type=float,
        default=0.05,
        help="intervalo em segundos entre linhas (padrao: 0.05)",
    )
    return parser.parse_args()


def carregar_linhas() -> list[str]:
    linhas = [
        linha.strip()
        for linha in ROTINA_ASSEMBLY.splitlines()
        if linha.strip()
    ]

    linhas.extend(("END", "RUN", "AUTO"))

    return linhas


def escolher_porta() -> str:
    portas = sorted(list_ports.comports(), key=lambda porta: porta.device)
    if not portas:
        raise RuntimeError("nenhuma porta serial encontrada")

    print("Portas seriais disponiveis:")
    for indice, porta in enumerate(portas, start=1):
        descricao = f" - {porta.description}" if porta.description else ""
        print(f"  {indice}. {porta.device}{descricao}")

    while True:
        resposta = input("Escolha o numero da porta: ").strip()
        try:
            indice = int(resposta)
        except ValueError:
            print("Informe um numero valido.")
            continue

        if 1 <= indice <= len(portas):
            return portas[indice - 1].device

        print("Escolha um numero da lista.")


def enviar_programa(porta: str, linhas: list[str], intervalo: float) -> None:
    if intervalo < 0:
        raise ValueError("o intervalo nao pode ser negativo")

    with serial.Serial(porta, BAUD_RATE, timeout=1, write_timeout=2) as conexao:
        time.sleep(2)
        for numero, linha in enumerate(linhas, start=1):
            conexao.write((linha + "\n").encode("ascii"))
            conexao.flush()
            print(f"[{numero:03d}/{len(linhas):03d}] {linha}")
            if intervalo:
                time.sleep(intervalo)


def main() -> int:
    argumentos = parse_arguments()

    try:
        porta = argumentos.porta or escolher_porta()
        linhas = carregar_linhas()
        enviar_programa(porta, linhas, argumentos.intervalo)
    except (OSError, RuntimeError, ValueError, serial.SerialException) as erro:
        print(f"Erro ao enviar a rotina: {erro}", file=sys.stderr)
        return 1

    print("Rotina enviada com sucesso.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())