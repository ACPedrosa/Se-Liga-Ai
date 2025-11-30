# Captura e Registro de Dados do Jogo via Porta Serial (Arduino)

Este script Python realiza a **detecção automática do Arduino**, estabelece uma conexão via porta serial e **captura blocos de registros** enviados pelo Arduino.

Os dados são armazenados em um arquivo **.txt** com horário e data de log.

---

## ✅ Funcionalidades Principais

* **Detecção automática** da porta serial onde o Arduino está conectado.
* **Conexão estável** com tratamento de erros e tentativas de reconexão.
* Criação automática de **arquivo de log**, com nome baseado na data e hora.
* Captura estruturada de blocos, iniciando em:

    ```
    === INÍCIO DO REGISTRO - TENTATIVA X ===
    ```

    e encerrando em:

    ```
    === FIM DO REGISTRO ===
    ```
* Armazenamento organizado, com **separadores** entre registros.

---

## 🚀 Como o Script Funciona

O script é dividido em funções que gerenciam cada etapa do processo:

| Etapa | Descrição | Palavras-chave de busca | Função de Exemplo |
| :--- | :--- | :--- | :--- |
| **1. Encontrar o Arduino** | O script verifica portas que contenham: `arduino`, `ch340`, `usb serial`. | `arduino`, `ch340`, `usb serial` | `def encontrar_arduino():` |
| **2. Estabelecer Conexão Serial** | Tenta conectar repetidamente até conseguir. | `baudrate`, `timeout` | `def conectar_serial(baudrate=9600, timeout=2):` |
| **3. Criar o Arquivo de Log** | Gera automaticamente um nome de arquivo datado. | `log_registros`, `YYYYMMDD`, `HHMMSS` | `def iniciar_log():` |
| **4. Capturar Blocos de Registro** | Lê linha por linha, detectando início e fim dos blocos. | `ler linha`, `detectar início e fim` | `def capturar_blocos(ser, arquivo):` |

---

### Requisitos
Python 3.8+

Biblioteca pyserial:

```bash

pip install pyserial

```

### Como Executar

```bash
python registro_jogo_pyserial.py

```
### Autora

Ana Caroline Pedrosa da Silva

Trabalho de Conclusão de Curso – Instituto Federal do Paraná (IFPR)

Projeto: “Se Liga Aí!”: Plataforma Embarcada para Aprendizagem de Crianças Cegas