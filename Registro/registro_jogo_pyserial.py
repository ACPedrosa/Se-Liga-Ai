"""
    Registro dos dados do jogo
    Objetivo Capturar e registrar os dados do jogo enviados via porta serial pelo Arduino.
    @author: Ana Caroline Pedrosa
"""

import serial
import serial.tools.list_ports
import time

def encontrar_arduino():
    """
    Procura a porta do Arduino conectada ao computador.
        Identifica dispositivos de nome:
            - 'arduino'
            - 'ch340'
            - 'usb serial'

        Retorna:
            str: nome da porta, ex.: '/dev/ttyUSB0'
            None: se nenhum dispositivo for encontrado
    """
    portas = serial.tools.list_ports.comports()
    for porta in portas:
        desc = porta.description.lower()
        print(desc, '---', porta.device)
        if 'arduino' in desc or 'ch340' in desc or 'usb serial' in desc:
            return porta.device
    return None

def conectar_serial(baudrate=9600, timeout=2):
    """
    Tenta se conectar ao Arduino até ter sucesso
        Parâmetros:
            baudrate (int): taxa de comunicação serial.
            timeout (int): tempo limite de leitura.

        Retorna:
            serial.Serial: objeto da conexão.
    """
    while True:
        porta = encontrar_arduino()
        if porta:
            try:
                print(f"Conectando à {porta}...")
                ser = serial.Serial(porta, baudrate, timeout=timeout)
                time.sleep(2)
                print("Conectado!")
                return ser
            except:
                print("Erro ao abrir porta. Tentando de novo em 3s...")
        else:
            print("Arduino não encontrado...")
        time.sleep(3)

def iniciar_log():
    """
    Cria um arquivo de log e escreve cabeçalho contendo a data e hora de início.
        Retorna:
            file: um arquivo de log
    """
    nome_arquivo = f"log_registros_{time.strftime('%Y%m%d_%H%M%S')}.txt"
    arquivo = open(nome_arquivo, "w", encoding="utf-8")

    arquivo.write("==============================================\n")
    arquivo.write("     REGISTRO DE TENTATIVAS DO SISTEMA\n")
    arquivo.write(time.strftime("     Gerado em %d/%m/%Y às %H:%M:%S\n"))
    arquivo.write("==============================================\n\n")

    return arquivo

def capturar_blocos(ser, arquivo):
    """
    Captura blocos de dados enviados pelo Arduino pela porta serial
        Parâmetros:
            ser (serial.Serial): conexão serial ativa.
            arquivo (file): arquivo de log aberto.
    """
    print("Capturando registros...\nPressione Ctrl+C para parar.\n")

    dentro_do_bloco = False
    buffer = []

    try:
        while True:
            if ser.in_waiting > 0:
                linha = ser.readline().decode("utf-8", errors="ignore").strip()

                if not linha:
                    continue

                print(linha)

                # Verifica início do bloco
                if linha.startswith("=== INÍCIO DO REGISTRO"):
                    dentro_do_bloco = True
                    buffer = [linha]
                    continue

                # Verifica fim do bloco
                if "=== FIM DO REGISTRO ===" in linha:
                    buffer.append(linha)

                    # Escreve o bloco no arquivo
                    arquivo.write("\n".join(buffer))
                    arquivo.write("\n\n----------------------------------------------\n\n")
                    arquivo.flush()

                    dentro_do_bloco = False
                    buffer = []
                    continue

                # Se estiver no bloco, armazena contueudo
                if dentro_do_bloco:
                    buffer.append(linha)

    except KeyboardInterrupt:
        print("\nFinalizando captura...")
    finally:
        arquivo.close()
        ser.close()


if __name__ == "__main__":
    ser = conectar_serial()
    arquivo = iniciar_log()
    capturar_blocos(ser, arquivo)
