/*
  Se Liga Aí - codigo da plataforma
  @authors: Ana Caroline Pedrosa e Silva & Thiago Berticelli Ló
  version: 2.5 
*/

// --- Librares ---
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <stdlib.h>
#include <time.h>

// --- DFPlayer - inicialização ---
SoftwareSerial mySerial(10, 11);  // RX, TX
DFRobotDFPlayerMini myDFPlayer;

//--- LED Registro (para visibilidade do estado do sistema) e variáveis de controle de registro ---
#define LED_REGISTRO 5

unsigned long tempoInicioRegistro = 0;
bool registroAtivo = false;

// --- Variáveis e estruturas de correção ---
#define N_BUFFER 6
#define TAMANHO_MAX 50

char bufferCorrecao[N_BUFFER][TAMANHO_MAX];
char bufferIncorreto[N_BUFFER][TAMANHO_MAX];
int quantIncorretos = 0;

// --- Botões de configuração - {btn_teste, btn_map, btn_confirma}
const int n_config = 3;
const int pinButtonsConfig[n_config] = {46, 7, 6};

// --- Botões de Dificuldade e Seleção de Jogo - {btn_dificuldade01,btn_dificuldade02, btn_dificuldade03 } ---
const int n_dificuldade_selecao = 3;
const int pinButtonsDificuldadeSelecao[n_dificuldade_selecao] = {47, 48, 49};

// --- Configuração dos botões - referentes aos audios de conceitos ---
const int tamanho_buttons = 12;
const int pinButtons[tamanho_buttons] = {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33};

// --- Configuração para os pinos de conexão ---
const int numero_pinos = 6;
const int aPins[numero_pinos] = {34, 35, 36, 37, 38, 39};  // Fileira A - sons de perguntas ou estímulos iniciais.
const int bPins[numero_pinos] = {40, 41, 42, 43, 44, 45};  // Fileira B - sons de respostas.

// --- Áudios - um número é associado a cada audio para correção das conexões ---
const int tam_audios = 6;
int vet_audios01[tam_audios] = {1, 2, 3, 4, 5, 6};
int vet_audios02[tam_audios] = {7, 8, 9, 10, 11, 12};

// --- Variáveis - Seleção do Jogo e Nível de dificuldade  ---
int total_jogos = 98;
int intro_atual = 0;
int jogo_selecionado = 0;
int nivel_selecionado = 1; // por padrão o jogo permanece no primeiro nível

// --- Variáveis -  Registro e Mapeamento --
int tentativas = 0;
unsigned long ultimo_mapeamento = 0;

// --- Variáveis para pastas de audios de configuração e interação ---
const int pasta_intro_mapeamento = 99;
const int pasta_feedback = 98;

// --- Flags para controle de transição de estados ---
unsigned long tempoFimAudio = 0;

int indiceFeedback = 0; 
int pastaAudioAtual = -1;
int numeroAudioAtual = -1; 

bool introTocada = false;
bool aguardandoAudio = false;
bool entradaAudioExecutada = false; //Flag para garantir áudio inicial do estado toque apenas uma vez

//Variáveis para debounce dos botões da plataforma
const unsigned long intervalo_debaunce = 50;
const int max_pinos_rastreados = 80;

// Variável para  verificar os estado da fsm
int estadoAnterior = -1;

//Variáveis para controle de ST_INTRO
unsigned long tempo_inicio_intro = 0;
const unsigned long tempo_limite = 40000;

// Variáveis para controle da reprodução dos Audios
#define TAM_FILA 20


/* 
  ---------------------------------------------------------------------
 | Classe Par                                                         |
 |                                                                    |
 | Quando uma nova conexão é realizada, é instanciado um objeto       |
 | da classe Par que guarda informações sobre a conexão.              | 
 |   Informações:                                                     |
 |   - pergunta: N° do pino correspondente a pergunta - origem        |
 |     - resposta: N° do pino correspondenre a resposta - destino     |
  ---------------------------------------------------------------------
*/
class Par {
  public:
    int pergunta;
    int resposta;

    Par() {
      pergunta = 0;
      resposta = 0;
    }

    Par(int pergunta, int resposta) {
      this->pergunta = pergunta;
      this->resposta = resposta;
    }

    bool igual(Par valor_par) {
      return (pergunta == valor_par.pergunta && resposta == valor_par.resposta);
    }
};

// --- Gabarito de pares corretos ---
Par pares_corretos[numero_pinos] = {
  Par(1, 7),
  Par(2, 8),
  Par(3, 9),
  Par(4, 10),
  Par(5, 11),
  Par(6, 12)
};

// Conexões feitas pelo usuário
Par conexoes_usuario[numero_pinos];

// Struct para Audio não bloquenates
struct AudioItem {
  int pasta;
  int arquivo;
};

AudioItem fila[TAM_FILA];
volatile int fila_inicio = 0;
volatile int fila_fim = 0;
bool audioTocando = false;

// Estrutura para rastrear o estado de cada botão
struct EstadoBotao {
    int pino;
    bool ultimoEstadoLeitura;             // O último estado lido do pino (HIGH ou LOW)
    unsigned long ultimoTempoMudanca;    // O tempo em que da última mudança de estado 
    bool estadoEstavel;                   // O estado do botão
    bool foiAcionado;                     // indica o evento de acionamento do botão
};

EstadoBotao rastreamentoBotoes[max_pinos_rastreados];
int contadorPinosRastreados = 0;

enum EstadosSeLigaAi{
  ST_INTRO = 0,
  ST_AUDIODESCRICAO = 1,
  ST_SELECAO_JOGO = 2,
  ST_SELECAO_DIFICULDADE = 3,
  ST_JOGO_PRONTO = 4,
  ST_TESTE = 5,
  ST_FEEDBACK = 6,
  ST_REGISTRO = 7,
};

EstadosSeLigaAi estadoAtual;
EstadosSeLigaAi proximoEstado;

//Funções para debaounce do botão
void registrarPino(int pino) {
    if (contadorPinosRastreados >= max_pinos_rastreados) return;
    
    // Inicializa a nova estrutura
    rastreamentoBotoes[contadorPinosRastreados].pino = pino;
    rastreamentoBotoes[contadorPinosRastreados].ultimoEstadoLeitura = digitalRead(pino);
    rastreamentoBotoes[contadorPinosRastreados].ultimoTempoMudanca = millis();
    rastreamentoBotoes[contadorPinosRastreados].estadoEstavel = rastreamentoBotoes[contadorPinosRastreados].ultimoEstadoLeitura;
    rastreamentoBotoes[contadorPinosRastreados].foiAcionado = false;
    
    contadorPinosRastreados++;
}

void atualizarEstadoBotoes() {
    for (int i = 0; i < contadorPinosRastreados; i++) {
        EstadoBotao* estado = &rastreamentoBotoes[i]; 

        bool leituraAtual = digitalRead(estado->pino);

        if (leituraAtual != estado->ultimoEstadoLeitura) {
            estado->ultimoTempoMudanca = millis(); 
        }

        if (leituraAtual != estado->estadoEstavel) {
            if (millis() - estado->ultimoTempoMudanca >= intervalo_debaunce) {
                
                estado->estadoEstavel = leituraAtual;

                if (estado->estadoEstavel == LOW) {
                    estado->foiAcionado = true; 
                }
            }
        }
        
        estado->ultimoEstadoLeitura = leituraAtual;
    }
}

bool verificarAcionamentoBotao(int pino) {
    for (int i = 0; i < contadorPinosRastreados; i++) {
        if (rastreamentoBotoes[i].pino == pino) {
            if (rastreamentoBotoes[i].foiAcionado) {
                rastreamentoBotoes[i].foiAcionado = false; // Consome o evento
                return true;
            }
            return false;
        }
    }
    return false;
}

//  Funções para Inicialização de Pinos
void inicializarPinos() {
  for(int i = 0; i < n_config; i++){
    pinMode(pinButtonsConfig[i], INPUT_PULLUP);
    registrarPino(pinButtonsConfig[i]);
  }

  for (int i = 0; i < tamanho_buttons; i++) {
    pinMode(pinButtons[i], INPUT_PULLUP);
    registrarPino(pinButtons[i]);
  }

  for(int i = 0; i < n_dificuldade_selecao; i++){
    pinMode(pinButtonsDificuldadeSelecao[i], INPUT_PULLUP);
    registrarPino(pinButtonsDificuldadeSelecao[i]);
  }

  for (int i = 0; i < numero_pinos; i++) {
    pinMode(aPins[i], INPUT);
    pinMode(bPins[i], INPUT_PULLUP);
  }

  pinMode(LED_REGISTRO, OUTPUT);
  digitalWrite(LED_REGISTRO, LOW);

  Serial.println("Pinos inicializados");
}

/*
 ----------------------------------------------------------------------
|  Funções para configuração dos Botões e dos audios de conceito      |
 ----------------------------------------------------------------------
*/

//embaralha os audios
void embaralhar_vet_audio(int vetor[], int tamanho) {
  randomSeed(analogRead(A0)); //usa entrada analógica para embraralhar
  for (int i = tamanho - 1; i > 0; i--) {
    int j = random(0, i + 1);
    int temp = vetor[i];
    vetor[i] = vetor[j];
    vetor[j] = temp;
  }
}

// Verifica botões de som 
void verificarBotoes() {
  for (int i = 0; i < tamanho_buttons; i++) {
    if (verificarAcionamentoBotao(pinButtons[i])) {
      tocarAudioDoBotao(i);
    }
  }
}

void enfileirarAudio(int pasta, int arquivo) {
  if (((fila_fim + 1) % TAM_FILA) == fila_inicio) {
    Serial.println("A fila está cheia");
    return;
  }
  fila[fila_fim].pasta = pasta;
  fila[fila_fim].arquivo = arquivo;
  fila_fim = (fila_fim + 1) % TAM_FILA;

  Serial.print("Fila: ");
  Serial.print(pasta);
  Serial.print("/");
  Serial.println(arquivo);
}

void checarEventosDFPlayer() {
  if (myDFPlayer.available()) {
    uint8_t tipo = myDFPlayer.readType();

    if (tipo == DFPlayerPlayFinished) {
      Serial.println("Audio ok - finalizado");
      audioTocando = false; //seta como falso para o príxmo audio

    } else if (tipo == DFPlayerError) {
      Serial.println("Erro no DF");
      audioTocando = false; 
    }
  }
}

// Função que toca o próximo áudio da fila se não houver nada tocando
void processarFilaAudio() {
  checarEventosDFPlayer();

  if (audioTocando) 
    return;

  if (fila_inicio == fila_fim) 
    return;

  static unsigned long ultimoPlay = 0;

  if (millis() - ultimoPlay < 150) 
    return; 

  ultimoPlay = millis();

  AudioItem item = fila[fila_inicio];

  Serial.print("Tocando: pastiha ");
  Serial.print(item.pasta);
  Serial.print(" arquivo ");
  Serial.println(item.arquivo);

  myDFPlayer.playFolder(item.pasta, item.arquivo);
  audioTocando = true;

  fila_inicio = (fila_inicio + 1) % TAM_FILA;
}

void atualizarAudio() {
  // Insere na filaa
  processarFilaAudio();
}

void tocarAudioDoBotao(int indiceBotao) {
  Serial.print("Botão pressionada ");
  Serial.println(indiceBotao);

  int pasta = jogo_selecionado; 
  int arquivo;

  if (indiceBotao < 6) {
    arquivo = vet_audios01[indiceBotao];
  } else {
    arquivo = vet_audios02[indiceBotao - 6];
  }

  Serial.print("Adicionando audio na pasta");
  Serial.print(pasta);
  Serial.print(", arquivo ");
  Serial.println(arquivo);

  enfileirarAudio(pasta, arquivo); 
}

/*
 ----------------------------------------------------------------------
 |  Funções para verificar conexões                                   |
 ----------------------------------------------------------------------
*/

// Verifica se todas as conexões estão conectadas fisicamente
bool verificarConexoes() {
  bool todasConectadas = true;

  for (int i = 0; i < numero_pinos; i++) {
    pinMode(aPins[i], OUTPUT);
    digitalWrite(aPins[i], LOW);
    delay(2); 

    bool conectado = false;

    for (int j = 0; j < numero_pinos; j++) {
      delay(5);
      if (digitalRead(bPins[j]) == LOW) {
        conexoes_usuario[i] = Par(vet_audios01[i], vet_audios02[j]);
        conectado = true;
        break;
      }
    }

    if (!conectado) {
      conexoes_usuario[i] = Par(vet_audios01[i], -1);
      todasConectadas = false;
    }

    pinMode(aPins[i], INPUT);
  }

  return todasConectadas;
}

// Avalia se todas as conexões estão corretas
bool todasCorretas() {
  for (int i = 0; i < numero_pinos; i++) {
    bool certo = false;
    for (int j = 0; j < numero_pinos; j++) {
      if (conexoes_usuario[i].igual(pares_corretos[j])) {
        certo = true;
        break;
      }
    }
    if (!certo) return false;
  }
  return true;
}

// Gera relatório das conexões - utilizado posteriormente no estado de registro
void gerarRelatorioConexoes() {
  for (int i = 0; i < numero_pinos; i++) {
    bool certo = false;
    for (int j = 0; j < numero_pinos; j++) {
      if (conexoes_usuario[i].igual(pares_corretos[j])) {
        certo = true;
        break;
      }
    }

    snprintf(
      bufferCorrecao[i], TAMANHO_MAX,
      "Par %d -> %d: %s",
      conexoes_usuario[i].pergunta,
      conexoes_usuario[i].resposta,
      certo ? "Correto" : "Incorreto"
    );

  }
}

// Obtém os pares de conexões incorretas - para contagem de conexões e debug
void obterParesIncorretos(){
    quantIncorretos = 0;

    for(int i = 0; i < N_BUFFER; i++){
      if(strstr(bufferCorrecao[i], "Incorreto" )!= NULL){
        strncpy(bufferIncorreto[quantIncorretos], bufferCorrecao[i], TAMANHO_MAX);
        bufferIncorreto[quantIncorretos][TAMANHO_MAX - 1] = '\0';
        quantIncorretos++;
      }
    }
}


/*
 ----------------------------------------------------------------------
 |  Funções para processar feedback                                   |
 ----------------------------------------------------------------------
*/

void processarFeedback(int nivel_selecionado, int quantIncorretos, char bufferIncorreto[][TAMANHO_MAX]) {
   if (todasCorretas()) {
    tocarAudioSequencial(pasta_feedback, 21); // “Parabéns! Todas as conexões estão certas.”
    return;
  }
  switch (nivel_selecionado) {
    case 1:
      feedbackFacil(bufferIncorreto, quantIncorretos);
      break;
    case 2:
      feedbackMedio(quantIncorretos);
      break;
    case 3:
      feedbackDificil();
      break;
    default:
      Serial.println("Estado do feedback - Nível inválido ou não selecionado.");
      break;
  }
}

//Função para reproduzir o feedback
void tocarAudioSequencial(int pasta, int arquivo) {
  myDFPlayer.playFolder(pasta, arquivo);
  delay(2000);
}

// Feedback fácil - com informações sobre os pares conexões erradas
void feedbackFacil(char bufferIncorreto[][TAMANHO_MAX], int quantIncorretos) {
  tocarAudioSequencial(pasta_feedback, 13); // "Vamos verificar as conexões."

  for (int i = 0; i < quantIncorretos; i++) {
    int num1 = 0, num2 = 0;
    sscanf(bufferIncorreto[i], "Par %d -> %d", &num1, &num2);

    tocarAudioSequencial(pasta_feedback, 14); // "A conexão"
    tocarAudioSequencial(pasta_feedback, num1); // número 01
    tocarAudioSequencial(pasta_feedback, 15); // "e"
    tocarAudioSequencial(pasta_feedback, num2); // número 02
    tocarAudioSequencial(pasta_feedback, 16); // "está errada"
  }
  tocarAudioSequencial(pasta_feedback, 20); // "Ajuste as conexões e tente novamente."
}

// Feedback medio - com informações sobre a quantidade de incorretas
void feedbackMedio(int quantIncorretos) {
  Serial.print("Médio - Quantidade de erros: ");
  Serial.println(quantIncorretos);

  tocarAudioSequencial(pasta_feedback, 13); // "Vamos verificar as conexões."
  tocarAudioSequencial(pasta_feedback, 17);

  if (quantIncorretos > 0 && quantIncorretos <= 6) {
    tocarAudioSequencial(pasta_feedback, quantIncorretos);
  }

  tocarAudioSequencial(pasta_feedback, 18); // "Tente novamente"
  tocarAudioSequencial(pasta_feedback, 20); // "Ajuste as conexões e tente novamente."
}

// Feedback difícil - com audio genérico
void feedbackDificil() {
  Serial.println("Difícil - mensagem genérica.");

  tocarAudioSequencial(pasta_feedback, 13); // "Vamos verificar as conexões."
  tocarAudioSequencial(pasta_feedback, 19); // "Alguma conexão está errada."
  tocarAudioSequencial(pasta_feedback, 20); // "Verifique e teste novamente."
}

/*
 ----------------------------------------------------------------------
 |  Função registro do log do sistema e para o estado de registro     |
 ----------------------------------------------------------------------
*/

void enviarRegistro() {
  Serial.print("=== INÍCIO DO REGISTRO - TENTATIVA ");
  Serial.print(tentativas++);
  Serial.println(" ===");

  // Envia o nível e quantidade de conexões incorretas
  Serial.print("NÍVEL DE DIFICULDADE: ");
  Serial.println(nivel_selecionado);
  Serial.print("QUANTIDADE DE INCORRETAS: ");
  Serial.println(quantIncorretos);

  // Se não houve erros
  if (quantIncorretos == 0) {
    Serial.println("TODAS AS CONEXÕES CORRETAS!");
  } else {
    Serial.println("LISTA DE CONEXÕES:");

    for (int i = 0; i < numero_pinos; i++) {
      Serial.print(" -> ");
      Serial.println(bufferCorrecao[i]);
    }
  }

  Serial.println("=== FIM DO REGISTRO ===");
  Serial.println();
}

void piscarLedRegistro() {
    static unsigned long ultimoAcionamento = 0;
    const unsigned long intervaloPisca = 300; 
    static bool estadoLED = false;

    //verifica se o estado de registro está ativo
    if (!registroAtivo) return; 

    if (millis() - ultimoAcionamento >= intervaloPisca) {
        ultimoAcionamento = millis(); 
        estadoLED = !estadoLED;       
        digitalWrite(LED_REGISTRO, estadoLED);
    }

    if (millis() - tempoInicioRegistro >= 3000) {
        registroAtivo = false;             
        digitalWrite(LED_REGISTRO, LOW);    
        Serial.println("Estado de regsitro - para pisca");
    }
}


/*
 ---------------------------------------------------------------------- 
 |  Funções voltadas para o mapeamento contextual                     |
 ----------------------------------------------------------------------
*/

void tocarMapeamentoDoEstado() {
  switch (estadoAtual) {
    case ST_SELECAO_JOGO:
      Serial.println("Navegação entre jogos - map");
      enfileirarAudio(pasta_intro_mapeamento, 22);
      enfileirarAudio(pasta_intro_mapeamento, 23);
      enfileirarAudio(pasta_intro_mapeamento, 24);
    break;
    case ST_SELECAO_DIFICULDADE:
      Serial.println("Seleção de dificuldade - map");
      enfileirarAudio(pasta_intro_mapeamento, 25);
      enfileirarAudio(pasta_intro_mapeamento, 26);
      enfileirarAudio(pasta_intro_mapeamento, 27);
    break;
    case ST_JOGO_PRONTO:
      Serial.println("Jogo pronto - map");
      enfileirarAudio(pasta_intro_mapeamento, 12);
    break;
    default:
      Serial.println("Mapeamento padrão");
      enfileirarAudio(pasta_intro_mapeamento, 5);
    break;
  }
}

void verificar_mapeamento() {
  if (verificarAcionamentoBotao(pinButtonsConfig[1]) && millis() - ultimo_mapeamento > 2000) {
    ultimo_mapeamento = millis();
    tocarMapeamentoDoEstado();
  }
}

// Função para tocar os audios inciais de cada estado
void tocarAudioEntrada(EstadosSeLigaAi s) {
    switch (s) {
        case ST_INTRO:
            enfileirarAudio(pasta_intro_mapeamento, 1);
            break;

        case ST_AUDIODESCRICAO:
            for (int i = 11; i <= 21; i++) {
                enfileirarAudio(pasta_intro_mapeamento, i);
            }
            break;

        case ST_SELECAO_JOGO:
            enfileirarAudio(pasta_intro_mapeamento, 4);
            break;

        case ST_SELECAO_DIFICULDADE:
            enfileirarAudio(pasta_intro_mapeamento, 5);
            break;

        case ST_JOGO_PRONTO:
            enfileirarAudio(pasta_intro_mapeamento, 6);
            break;

        case ST_TESTE:
            break;

        case ST_FEEDBACK:
            enfileirarAudio(pasta_intro_mapeamento, 13);
            break;

        case ST_REGISTRO:
            break;
    }
}

// Compara o estado atual com o estado anteior
bool verificarEntradaDeEstado(int estadoAtual) {
    if (estadoAtual != estadoAnterior) {
        estadoAnterior = estadoAtual; 
        return true;                 
    }
    return false;
}

void processarEntradaEstado(EstadosSeLigaAi estado) {
    if (!entradaAudioExecutada) {
        Serial.print("Entrada: ");
        Serial.println(estado);

        tocarAudioEntrada(estado);

        if (estado == ST_INTRO) {
            tempo_inicio_intro = millis();
        }
    
        entradaAudioExecutada = true; 
    }
}

/*
 ----------------------------------------------------------------------
 | Funções dos estados do sistema (entrada/execução separadas)
    - Entrada: executada somente uma vez no momento da transição
    - Execução: função que irá ser executada continuamente no loop 
 ----------------------------------------------------------------------
*/

// --- ST_INTRO ---
void entradaEstadoIntro() {
    processarEntradaEstado(ST_INTRO);
}

void execucaoEstadoIntro() {
    //  Se o btn_map for pressionado então muda de estado --> ST_AUDIODESCRICAO
    if (verificarAcionamentoBotao(pinButtonsConfig[1])) {
        proximoEstado = ST_AUDIODESCRICAO;
        Serial.println("ST_INTRO --> usuario solicitou mapeamento.");
        return;
    }

    // Se o tempo passar então continua o jogo
    if (millis() - tempo_inicio_intro > tempo_limite) {
        proximoEstado = ST_SELECAO_JOGO;
        Serial.println("ST_INTRO --> tempo esgotado.");
        return;
    }
}

// --- ST_AUDIODESCRICAO ---
void entradaEstadoAudiodescricao() {
    processarEntradaEstado(ST_AUDIODESCRICAO);
}

void execucaoEstadoAudiodescricao() {
    if (!audioTocando && fila_inicio == fila_fim) {
        proximoEstado = ST_SELECAO_JOGO;
        Serial.println("ST_AUDIODESCRICAO --> fila de audios vazia");
    }
}

// --- ST_SELECAO_JOGO ---
void entradaEstadoSelecaoJogo() {
    processarEntradaEstado(ST_SELECAO_JOGO);
}

void execucaoEstadoSelecaoJogo() {
    verificar_mapeamento(); //verifica a todo momento para oferecer ao usuário uma ajuda contextual

    if (verificarAcionamentoBotao(pinButtonsDificuldadeSelecao[0])) {
        if (intro_atual > 0) {
            intro_atual--;
            Serial.print("Jogo anterior: ");
            Serial.println(intro_atual);
            enfileirarAudio(intro_atual, 13);
        }

    } else if (verificarAcionamentoBotao(pinButtonsConfig[2])) {
        jogo_selecionado = intro_atual;
        proximoEstado = ST_SELECAO_DIFICULDADE;
        Serial.print("Jogo selecionado: ");
        Serial.println(jogo_selecionado);

    } else if (verificarAcionamentoBotao(pinButtonsDificuldadeSelecao[2])) {
        if (intro_atual < total_jogos) {
            intro_atual++;
            Serial.print("Próximo jogo: ");
            Serial.println(intro_atual);
            enfileirarAudio(intro_atual, 13);
        } else {
            Serial.println("Limite - sem jogo");
        }
    }
}

// --- ST_SELECAO_DIFICULDADE ---
void entradaEstadoDificuldade() {
    processarEntradaEstado(ST_SELECAO_DIFICULDADE);
}

// Função que verifica o nível selecionado pelo aluno - btn_confirma
int verificarNivel() {
    for (int i = 0; i < n_dificuldade_selecao; i++) {
        if (verificarAcionamentoBotao(pinButtonsDificuldadeSelecao[i])) {
            nivel_selecionado = i + 1;
            enfileirarAudio(pasta_feedback, nivel_selecionado);
            Serial.print("Nível selecionado: ");
            Serial.println(nivel_selecionado);
            return nivel_selecionado;
        }
    }
    return nivel_selecionado;
}

void execucaoEstadoDificuldade() {
    verificar_mapeamento(); 

    // Verifica o Nível
    verificarNivel();

    // Confirma a seleção com o btn_confirma 
    if (verificarAcionamentoBotao(pinButtonsConfig[2])) {
        Serial.print("Nível: ");
        Serial.print(nivel_selecionado);
        Serial.println(" Nivel confirmado");
        proximoEstado = ST_JOGO_PRONTO;
    }
}

// --- ST_JOGO_PRONTO ---
void entradaEstadoJogoPronto() {
    if (!introTocada) {
        processarEntradaEstado(ST_JOGO_PRONTO);
        introTocada = true;
    } else {
        processarEntradaEstado(ST_JOGO_PRONTO); 
    }
}

void execucaoEstadoJogoPronto() {
    verificarBotoes(); 

    if (verificarAcionamentoBotao(pinButtonsConfig[0])) {
        Serial.println("Botão de teste presxionado");
        proximoEstado = ST_TESTE;
    }
}

// --- ST_TESTE ---
void entradaEstadoTeste() {
    processarEntradaEstado(ST_TESTE);
}

void execucaoEstadoTeste() {
    Serial.println("Vamos verificar as conexões");
    if (verificarConexoes()) {
        Serial.println("Todas as conexões estão completas - ST_FEEDBACK");
        proximoEstado = ST_FEEDBACK;
    } else {
        Serial.println("Há conexões incomepletas - ST_JOGO_PRONTO");
        enfileirarAudio(pasta_feedback, 22); 
        proximoEstado = ST_JOGO_PRONTO;
    }
}

// --- ST_FEEDBACK ---
void entradaEstadoFeedback() {
    if (!entradaAudioExecutada) {

        gerarRelatorioConexoes();
        obterParesIncorretos();
        
        processarFeedback(nivel_selecionado, quantIncorretos, bufferIncorreto);

        entradaAudioExecutada = true;
    }
}

void execucaoEstadoFeedback() {
    if (!audioTocando && fila_inicio == fila_fim) {
        if (todasCorretas()) {
            Serial.println("Paravéns, todas as conexçoes estão corretas");
            proximoEstado = ST_REGISTRO;
        } else {
            Serial.println("Ainda há conexões incorretas - ST_JOGO_PRONTO");
            proximoEstado = ST_JOGO_PRONTO;
        }
    }
}

// --- ST_REGISTRO ---
void entradaEstadoRegistro() {
    if (!entradaAudioExecutada) {

        // --- Configuração da Próxima Rodada ---
        registroAtivo = true;
        tempoInicioRegistro = millis();

        introTocada = false;
        
        // Embaralha os audios novamente e atualiza gabarito
        embaralhar_vet_audio(vet_audios01, tam_audios);
        embaralhar_vet_audio(vet_audios02, tam_audios);

        for (int i = 0; i < numero_pinos; i++) {
            pares_corretos[i] = Par(vet_audios01[i], vet_audios02[i]);
        }
        Serial.println("Novo gabarito de áudios para a próxima rodada");

        Serial.println("Enviando registro...");
        enviarRegistro();

        entradaAudioExecutada = true;
    }
}

void execucaoEstadoRegistro() {
    if (!registroAtivo) {
        Serial.println("Tudo certo - ST_SELECAO_JOGO");
        proximoEstado = ST_SELECAO_JOGO;
    }
}


/*
 ----------------------------------------------------------------------
 | Função do FSM principal (gera transições e controla entradas)      |
 ----------------------------------------------------------------------
*/
void fsm() {
    // Executa uma unicaa vez - entrada do estado
    if (verificarEntradaDeEstado((int)estadoAtual)) {
        switch (estadoAtual) {
            case ST_INTRO: 
              entradaEstadoIntro(); 
            break;
            case ST_AUDIODESCRICAO: 
              entradaEstadoAudiodescricao(); 
            break;
            case ST_SELECAO_JOGO: 
              entradaEstadoSelecaoJogo(); 
            break;
            case ST_SELECAO_DIFICULDADE: 
              entradaEstadoDificuldade(); 
            break;
            case ST_JOGO_PRONTO: 
              entradaEstadoJogoPronto(); 
            break;
            case ST_TESTE: 
              entradaEstadoTeste(); 
            break;
            case ST_FEEDBACK: 
              entradaEstadoFeedback(); 
            break;
            case ST_REGISTRO: 
              entradaEstadoRegistro(); 
            break;
        }
    }
    // executa de forma contínua - execucao do estai
    switch (estadoAtual) {
        case ST_INTRO: 
          execucaoEstadoIntro(); 
        break;
        case ST_AUDIODESCRICAO: 
          execucaoEstadoAudiodescricao(); 
        break;
        case ST_SELECAO_JOGO: 
          execucaoEstadoSelecaoJogo(); 
        break;
        case ST_SELECAO_DIFICULDADE: 
          execucaoEstadoDificuldade(); 
        break;
        case ST_JOGO_PRONTO: 
          execucaoEstadoJogoPronto(); 
        break;
        case ST_TESTE: 
          execucaoEstadoTeste(); 
        break;
        case ST_FEEDBACK: 
          execucaoEstadoFeedback(); 
        break;
        case ST_REGISTRO: 
          execucaoEstadoRegistro(); 
        break;
    }

    //Log e transição entre estados
    if (proximoEstado != estadoAtual) {
        Serial.print("Transição: ");
        Serial.print(estadoAtual);
        Serial.print(" --> ");
        Serial.println(proximoEstado);
        
        //Configura as variáveis para a entrada em um outro estado
        entradaAudioExecutada = false;
        estadoAtual = proximoEstado;
    }
}

/*
 ----------------------------------------------------------------------
 |  Principall - setup e loop                                         |
 ----------------------------------------------------------------------
*/

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  inicializarPinos();

  if (!myDFPlayer.begin(mySerial)) {
    Serial.println("DFPlayer Mini initialization failed!");
    while (true);
  }

  Serial.println("DFPlayer Mini ready.");
  myDFPlayer.volume(25);

  embaralhar_vet_audio(vet_audios01, tam_audios);
  embaralhar_vet_audio(vet_audios02, tam_audios);

  // Inicializa FSM
  estadoAtual = ST_INTRO;
  proximoEstado = ST_INTRO;
  estadoAnterior = -1; 

  Serial.println("Sistema inicializado");
}

void loop() {
  piscarLedRegistro();   
  atualizarEstadoBotoes();
  atualizarAudio();       
  fsm();  

}
