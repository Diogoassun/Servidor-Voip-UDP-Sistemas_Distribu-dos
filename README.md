# Projeto VoIP UDP - Sistemas Distribuídos

**Aluno:** Diogo Bandeira Assunção  
**Instituição:** UFC - Campus Quixadá  
**Tecnologias:** C++, UDP Sockets, PortAudio

---

## 1. Visão Geral
Este projeto consiste na implementação de um sistema de comunicação de voz sobre IP (VoIP) utilizando o protocolo **UDP**. O sistema é composto por um Cliente e um Servidor que realizam uma etapa de sinalização (handshake) antes de iniciarem o fluxo de mídia em tempo real.

## 2. Arquitetura de Dados (POJO/POD)
Para a representação das entidades de rede, foram utilizadas classes no padrão **POD** (*Plain Old Data*). A classe principal, `Chamada`, encapsula os metadados da sessão:
* **id_call:** Identificador único da transação.
* **origin / destination:** Strings que identificam os usuários.
* **timestamp_start:** Registro de tempo do início da chamada.
* **status:** Controle de estado (Ex: 1 para INVITE, 3 para OK).
* **audio_port:** Definição da porta para o streaming de áudio.

## 3. Serialização e Marshalling
A comunicação entre processos foi viabilizada através de uma camada manual de **Marshalling**, utilizando as classes `calloutputstream` e `callinputstream`.
* **Marshalling:** Converte os objetos da memória interna (RAM) em um formato binário padronizado (serializado) para transmissão via socket.
* **Unmarshalling:** Reconstrói os objetos no host de destino, garantindo a integridade dos dados bit a bit.
* **Manipulação de Bytes:** Embora as versões modernas do C++ possuam tipos específicos, este projeto utiliza `unsigned char` para representar os octetos brutos, garantindo que não haja interferência de sinal aritmético na integridade dos dados.

## 4. Processamento de Áudio (PortAudio)
A captura e reprodução de som utilizam a biblioteca **PortAudio**. Esta escolha permite uma abstração eficiente do hardware, realizando o *polling* automático das interfaces de áudio do Sistema Operacional.

### Parâmetros Técnicos:
Para garantir compatibilidade com hardwares modernos (testado em Dell G15), os parâmetros de áudio foram fixados em:
* **Sample Rate:** 44100 Hz (Frequência padrão de CD).
* **Frames Per Buffer:** 1024 amostras (otimizado para reduzir jitter e latência).
* **Formato:** PCM 16-bit (paInt16).

---

## 5. Como Compilar e Executar

### Pré-requisitos (Linux)
Instale a biblioteca de desenvolvimento do PortAudio:
```bash
sudo apt install portaudio19-dev

## ENTRAGA 2

## Relatório Técnico: Implementação de Middleware RMI para Sistema VoIP

**Instituição:** Universidade Federal do Ceará - Campus de Quixadá
**Disciplina:** Sistemas Distribuídos (QXD0043)
**Autor(s):** Diogo Bandeira Assunção, Leandro Rodrigues da Silva Junior

---

### 1. Visão Geral

Este relatório documenta o desenvolvimento do Trabalho 2 da disciplina de Sistemas Distribuídos, que consiste na reimplementação de um serviço de comunicação distribuída utilizando a Invocação Remota de Método (RMI). O escopo técnico abrange a criação de um middleware RMI proprietário, operando sob um protocolo de requisição-resposta, responsável por gerenciar o plano de controle (sinalização) de chamadas de um sistema VoIP.

### 2. Arquitetura do Sistema

A arquitetura foi dividida estritamente em duas camadas para isolar a complexidade de rede da regra de negócio:

* **Camada de Middleware (RMI):** Implementa os mecanismos de empacotamento e roteamento de mensagens. O cliente atua através do método `doOperation`, enquanto o servidor processa as requisições em loop contínuo utilizando os métodos `getRequest` e `sendReply`.

* **Cliente Stub (doOperation)**
* **Servidor Skeleton** (getRequest e sendReply)**

doOperation, getRequest e sendReply, construindo o pacote de mensagem com messageType, requestId, objectReference, methodId e arguments

* **Camada de Aplicação:** Contém a lógica de negócio estruturada em conformidade com o paradigma de Orientação a Objetos exigido, gerenciando as sessões de usuários e os dispositivos de áudio.

### 3. Modelagem Orientada a Objetos

A aplicação cumpre rigorosamente as especificações estruturais definidas nos requisitos do projeto.

**Entidades (Mínimo de 4)**

1. `DispositivoAudio`: Classe base de hardware.
2. `Microfone`: Especialização de dispositivo de captura.
3. `AltoFalante`: Especialização de dispositivo de reprodução.
4. `Usuario`: Entidade representante do nó de comunicação.
5. `SessaoVoIP`: Entidade gestora do estado da chamada.

**Composições do Tipo Extensão ("é-um" - Mínimo de 2)**

* A classe `Microfone` herda de `DispositivoAudio`.
* A classe `AltoFalante` herda de `DispositivoAudio`.

**Composições do Tipo Agregação ("tem-um" - Mínimo de 2)**

* A classe `Usuario` possui uma instância de `Microfone` e uma instância de `AltoFalante`.
* A classe `SessaoVoIP` agrega instâncias de `Usuario` (Origem e Destino).

**Métodos Remotos (Mínimo de 4)**
Os seguintes métodos estão mapeados no objeto remoto (`ServidorVoIPRemoto`) para invocação via middleware:

* `iniciarChamada(origem_dict, destino_dict)`
* `encerrarChamada(sessaoId)`
* `consultarParticipantes(sessaoId)`
* `pingServidor()`

### 4. Representação Externa de Dados e Passagem de Parâmetros

A serialização das mensagens do protocolo RMI (empacotamento de `messageType`, `requestId`, `objectReference`, `methodId` e `arguments`) foi implementada utilizando a biblioteca nativa `json`. O tráfego de objetos entre cliente e servidor nos argumentos ocorre via passagem por valor, convertendo as instâncias em dicionários (`to_dict()`) antes da serialização para o envio na rede, garantindo a representação externa independente da arquitetura de hardware das partes.

---

### 5. Justificativa Técnica: A Presença de Sockets na Implementação

A restrição documental "Não crie sockets nesse trabalho!" direciona-se, do ponto de vista de Sistemas Distribuídos, à **Camada de Aplicação**. O objetivo da exigência é forçar a utilização do paradigma RMI, ocultando os detalhes da rede da regra de negócio. Contudo, a análise da implementação revela e justifica a presença de sockets sob duas frentes fundamentais:

**1. Sockets na Construção do Middleware (Transporte Subjacente)**
Como a diretriz do trabalho exige a construção do próprio mecanismo RMI (protocolo de requisição-resposta customizado, implementando `doOperation`, `getRequest` e `sendReply`), em vez do uso de frameworks prontos (como Java RMI ou Pyro5), a utilização de sockets na camada de base é um imperativo técnico. O middleware precisa de acesso aos sockets de transporte do sistema operacional para enviar o payload JSON serializado na rede. A aplicação em si não manipula esses sockets, ela apenas invoca métodos remotos.

**2. Sockets no Plano de Dados (Streaming de Áudio via Multicast)**
A arquitetura implementada adota uma topologia híbrida justificada pela natureza do tráfego VoIP. Foi estabelecida a separação entre o **Plano de Controle** e o **Plano de Dados**:

* **Plano de Controle (RMI):** Toda a sinalização (autorização, configuração de porta, encerramento de sessão) ocorre via o middleware RMI síncrono que foi construído.
* **Plano de Dados (Multicast UDP):** O tráfego real de frames de áudio utiliza sockets UDP multicast paralelos.

**Justificativa da Abordagem:** O modelo arquitetural de Invocação Remota de Método é, por definição, síncrono e bloqueante (`wait` no cliente até o retorno do servidor). Transmitir fluxos contínuos de áudio de alta taxa de amostragem (ex: 48kHz, lendo buffers a cada poucos milissegundos) através de repetidas requisições bloqueantes de RMI introduziria um *overhead* de serialização e uma latência de rede que inviabilizariam o tempo real exigido pelo VoIP, gerando *jitter* inaceitável. A adoção do Multicast para a mídia, orquestrada pelo RMI para a sinalização, garante a integridade conceitual do sistema distribuído sem comprometer os requisitos temporais da aplicação de áudio, sendo esta a estratégia validada para este cenário limítrofe.
