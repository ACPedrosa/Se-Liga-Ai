"""
    Geração de Audios usando gTTS
    Objetivo: Tranformar texto em audio seguindo o padrão especificado pelo módulo de audio DFPlayer
    @author: Ana Caroline Pedrosa
"""

import os
from gtts import gTTS

#---------------------------------------------------------------------------
#| DFPlayer - funções para padronização com o formato aceito pelo dfplayer |
#---------------------------------------------------------------------------

def criar_pasta_dfplayer(numero):
    """
    Cria pastas numeradas, seguindo o padrão '01', '02',..., '100' 
        Parâmetros:
            numero : str ou int - numero informado,será convertido para formato '01', '02', etc.

        Retorna:
            str - nome da pasta criada/existente.
    """
    pasta = f"{int(numero):02d}"
    if not os.path.exists(pasta):
        os.mkdir(pasta)
    return pasta


def salvar_audio(texto, pasta, index):
    """
    Salva os audios, seguindo o padrão '001', '002',...,'078',..., '100' 
        Parâmetros:
            texto : str - texto que será convertido em áudio.
            pasta : str - pasta onde o arquivo será salvo.
            index : int - indice numérico do áudio
    """
    nome_arquivo = os.path.join(pasta, f"{index:03d}.mp3")
    gTTS(texto, lang='pt').save(nome_arquivo)
    print(f"Audio salvo: {nome_arquivo}")

# ---------------------------------------------------------------------------------------------------------------------------
# |Leitura e processamento do arquivo - verifica as tags utilizadas no arquivo e organiza os audios de acordo com a leitura |
# ---------------------------------------------------------------------------------------------------------------------------

def carregar_blocos(filepath):
    """
    Lê o arquivo, separa os textos pela tag /space e interpreta o restante das tags.
        Parâmetros:
            filepath : str -  caminho do arquivo .txt contendo o conteúdo estruturado.

        Retorna:
            header : str ou None
            description : str ou None
            text_game : list[str]
    """
    with open(filepath, "r", encoding="utf-8") as f:
        conteudo = f.read()

    # divide cada bloco utilizando a tag /spcae
    blocos = [b.strip() for b in conteudo.split("/space") if b.strip()]

    header = None
    description = None
    text_game = []

    for bloco in blocos:

        # /header
        if bloco.startswith("/header"):
            header = bloco.replace("/header", "").replace("{", "").replace("}", "").strip()
            continue

        # /description
        if bloco.startswith("/description"):
            description = bloco.replace("/description", "").replace("{", "").replace("}", "").strip()
            continue

        # /coment com comentário
        if bloco.startswith("/coment"):
            # remove linha de /coment
            linhas = bloco.split("\n")
            texto = "\n".join(linhas[1:]).strip()
            text_game.append(texto)
            continue


    return header, description, text_game


# ---------------------------------
# |Conversão de texto para audio  |
# --------------------------- -----

def gerar_audios():
    """
        Parte interativa para solicitar ao usuário o caminho do arquivo e o número da pasta
    """
    print("\n --- Iniciando o gerador de audios ---\n")

    arquivo = input("Digite o caminho do arquivo de texto (.txt): ").strip()
    numero_pasta = input("Digite o número da pasta no DFPlayer(Ex.: 3): ").strip()

    pasta = criar_pasta_dfplayer(numero_pasta)

    # Carrega tudo
    header, description, text_game = carregar_blocos(arquivo)

    print(f"\nEncontradas:")
    print(f"- {len(text_game)} texto do jogo")
    print(f"- Header: {header is not None}")
    print(f"- Description: {description is not None}\n")

    indice_audio = 1

    # ---- Gerar audios referentes ao jogo ----
    for texto in text_game:
        salvar_audio(texto, pasta, indice_audio)
        indice_audio += 1


    # ---- descrição e header (número fixo - por padrão) ----
    if description:
        salvar_audio(description, pasta, 13)

    if header:
        salvar_audio(header, pasta, 14)

    print("\nAudios gerados")
    print(f"Audios salvos em: {pasta}/")
    print("Audios do jogo começam em 001.mp3.")
    print("Descrição sempre em 013.mp3 e header em 014.mp3.\n")


if __name__ == "__main__":
    gerar_audios()
