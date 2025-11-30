## Ferramenta de Geração de Áudios para DFPlayer Mini

### Conversão Automática de Texto para Fala (TTS) para a Plataforma Embarcada "Se Liga Ai!"

Este script foi desenvolvido para **automatizar a criação de áudios MP3** utilizando a biblioteca **gTTS (Google Text-to-Speech)**.

A ferramenta converte textos estruturados em um arquivo **.txt** em arquivos de áudio organizados no **formato exigido pelo módulo DFPlayer Mini**.

Foi projetado especialmente para o projeto **"Se Liga Aí!"**, que integra tecnologias acessíveis ao processo de aprendizagem de crianças cegas.

---

### Estrutura do Arquivo de Entrada (.txt)

O arquivo deve seguir o padrão de blocos usando **tags de controle**, que indicam como o texto será interpretado:

/header {Texto do cabeçalho do jogo} 

/description {Descrição geral do jogo}

/space 
/coment {Comentário opcional} 
Texto do bloco 1

/space 
/coment {Comentário opcional} 
Texto do bloco 2

### Tags Disponíveis

| Tag | Função |
| :--- | :--- |
| **/header** | Define o **cabeçalho geral** (informações introdutórias). |
| **/description** | Texto descritivo do jogo (usado em **013.mp3**). |
| **/coment** | **Comentários ou notas** — são ignorados na conversão. |
| **/space** | Marca o início de um **novo bloco de texto** (gerando um novo áudio). |

---

### Funcionalidades do Script

* Criação automática das **pastas numeradas** no padrão DFPlayer (01, 02, 03...)
* A conversão de **cada bloco de texto** em um arquivo **.mp3**
* Tratamento automático de comentários para **evitar leitura indevida**
* Organização padronizada dos áudios:

| Arquivo | Conteúdo |
| :--- | :--- |
| **001.mp3 – 012.mp3** | Áudios dos **blocos do jogo** |
| **013.mp3** | **Descrição** do jogo |
| **014.mp3** | **Header** (cabeçalho) |

---

### 🛠️ Requisitos

* **Python 3.8+**
* Biblioteca **gTTS**

**Instalação:**

```bash
pip install gTTS
```

### Como Executar

1.  Prepare o arquivo **.txt** com as tags especificadas.
2.  Execute o script:

```bash
python gerar_audios_jogos.py
```

3. Informe:
O caminho do arquivo .txt

O número da pasta desejada para o DFPlayer

### Autora

Ana Caroline Pedrosa da Silva

Trabalho de Conclusão de Curso – Instituto Federal do Paraná (IFPR)

Projeto: “Se Liga Aí!”: Plataforma Embarcada para Aprendizagem de Crianças Cegas