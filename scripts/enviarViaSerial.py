#!/usr/bin/env python3
"""Envia a rotina Assembly do reservatorio para o Arduino."""

from __future__ import annotations

import argparse
import sys
import time
from pathlib import Path

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
ARQUIVO_ROTINA = Path(__file__).with_name("rotina_assembly.txt")
ARQUIVO_LOG = Path(__file__).with_name("log.txt")


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
    parser.add_argument(
        "--monitorar",
        action="store_true",
        help="mantem a porta aberta para acompanhar as respostas pelo script",
    )
    return parser.parse_args()


def carregar_linhas(caminho: Path) -> list[str]:
    conteudo = caminho.read_text(encoding="utf-8")
    linhas = []
    for linha in conteudo.splitlines():
        instrucao = linha.split(";", 1)[0].strip()
        if instrucao:
            linhas.append(instrucao)

    linhas.append("END")

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


def registrar_resposta(conexao: serial.Serial, arquivo_log) -> None:
    while conexao.in_waiting:
        resposta = conexao.readline().decode("utf-8", errors="replace")
        if not resposta:
            return
        print(resposta, end="")
        arquivo_log.write(resposta)
        arquivo_log.flush()


def enviar_programa(
    porta: str,
    linhas: list[str],
    intervalo: float,
    caminho_log: Path,
    monitorar: bool,
) -> None:
    if intervalo < 0:
        raise ValueError("o intervalo nao pode ser negativo")

    with (
        serial.Serial(porta, BAUD_RATE, timeout=0.1, write_timeout=2) as conexao,
        caminho_log.open("a", encoding="utf-8") as arquivo_log,
    ):
        time.sleep(2)
        registrar_resposta(conexao, arquivo_log)

        for numero, linha in enumerate(linhas, start=1):
            conexao.write((linha + "\n").encode("ascii"))
            conexao.flush()
            print(f"[{numero:03d}/{len(linhas):03d}] {linha}")
            if intervalo:
                time.sleep(intervalo)
            registrar_resposta(conexao, arquivo_log)

        if not monitorar:
            print(
                "Envio concluido. A porta sera liberada; abra o monitor serial "
                "do Arduino IDE e envie RUN ou AUTO."
            )
            return

        print(f"Monitorando respostas; gravando em {caminho_log}. Pressione Ctrl+C para sair.")
        try:
            while True:
                resposta = conexao.readline().decode("utf-8", errors="replace")
                if resposta:
                    print(resposta, end="")
                    arquivo_log.write(resposta)
                    arquivo_log.flush()
        except KeyboardInterrupt:
            print("\nMonitoramento encerrado.")


def main() -> int:
    argumentos = parse_arguments()

    try:
        porta = argumentos.porta or escolher_porta()
        linhas = carregar_linhas(ARQUIVO_ROTINA)
        enviar_programa(
            porta,
            linhas,
            argumentos.intervalo,
            ARQUIVO_LOG,
            argumentos.monitorar,
        )
    except (OSError, RuntimeError, ValueError, serial.SerialException) as erro:
        print(f"Erro ao enviar a rotina: {erro}", file=sys.stderr)
        return 1

    print("Rotina enviada com sucesso.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())