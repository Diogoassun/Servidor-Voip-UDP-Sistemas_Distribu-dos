# Projeto VoIP UDP - Sistemas Distribuídos

**Aluno:** Diogo Bandeira Assunção e Leandro Rodrigues da Silva Júnior
**Instituição:** UFC - Campus Quixadá
**Tecnologias:** C++, UDP Sockets, PortAudio

---

##  Sumário

* [Entrega 1](#-entrega-1)
* [Entrega 2](#-entrega-2)
* [Visão Geral](#1-visão-geral)
* [Arquitetura do Sistema](#2-arquitetura-do-sistema)
* [Modelagem Orientada a Objetos](#3-modelagem-orientada-a-objetos)
* [Representação Externa de Dados](#4-representação-externa-de-dados-e-passagem-de-parâmetros)
* [Compilação](#5-como-compilar-e-executar)

---

#  Entrega 1

<details>
<summary><strong>Expandir conteúdo da Entrega 1</strong></summary>

## 1. Visão Geral

Este projeto consiste na implementação de um sistema de comunicação de voz sobre IP (VoIP) utilizando o protocolo **UDP**. O sistema é composto por um Cliente e um Servidor que realizam uma etapa de sinalização (*handshake*) antes de iniciarem o fluxo de mídia em tempo real.

---

## 2. Arquitetura de Dados (POJO/POD)

Para a representação das entidades de rede foram utilizadas classes no padrão **POD (Plain Old Data)**.

### Estrutura principal: `Chamada`

| Campo             | Descrição                        |
| ----------------- | -------------------------------- |
| `id_call`         | Identificador único da transação |
| `origin`          | Usuário de origem                |
| `destination`     | Usuário de destino               |
| `timestamp_start` | Registro do início da chamada    |
| `status`          | Controle do estado da chamada    |
| `audio_port`      | Porta utilizada no streaming     |

---

## 3. Serialização e Marshalling

A comunicação entre processos foi implementada através de uma camada manual de **Marshalling**, utilizando as classes:

* `calloutputstream`
* `callinputstream`

### Funções

| Processo             | Função                                                   |
| -------------------- | -------------------------------------------------------- |
| Marshalling          | Converte objetos em formato binário                      |
| Unmarshalling        | Reconstrói os objetos recebidos                          |
| Manipulação de Bytes | Usa `unsigned char` para preservar integridade dos dados |

---

## 4.  Processamento de Áudio (PortAudio)

A captura e reprodução utilizam a biblioteca **PortAudio**, permitindo abstração do hardware e gerenciamento automático das interfaces de áudio.

### Parâmetros Técnicos

| Parâmetro         | Valor                  |
| ----------------- | ---------------------- |
| Sample Rate       | 44100 Hz               |
| Frames per Buffer | 1024                   |
| Formato           | PCM 16-bit (`paInt16`) |

---

## 5.  Como Compilar e Executar

### Pré-requisitos (Linux)

```bash
sudo apt install portaudio19-dev
```

</details>

---

#  Entrega 2

<details>
<summary><strong>Expandir conteúdo da Entrega 2</strong></summary>

## 1. Visão Geral

Este relatório documenta o desenvolvimento do Trabalho 2 da disciplina de Sistemas Distribuídos, que consiste na reimplementação de um serviço de comunicação distribuída utilizando **Invocação Remota de Método (RMI)**.

O sistema implementa um middleware proprietário operando sob um protocolo de requisição-resposta para gerenciamento do plano de controle de chamadas VoIP.

---

## 2.  Arquitetura do Sistema

A arquitetura foi dividida em duas camadas:

### Middleware RMI

Responsável por empacotamento e roteamento das mensagens.

**Componentes:**

* Cliente Stub (`doOperation`)
* Servidor Skeleton (`getRequest`)
* Resposta (`sendReply`)

Pacote de comunicação:

```text
messageType
requestId
objectReference
methodId
arguments
```

---

### Camada de Aplicação

Responsável pela lógica de negócio:

* gerenciamento de usuários
* gerenciamento das sessões
* controle dos dispositivos de áudio

---

## 3.  Modelagem Orientada a Objetos

### Entidades

| Classe             | Função                   |
| ------------------ | ------------------------ |
| `DispositivoAudio` | Classe base de hardware  |
| `Microfone`        | Captura de áudio         |
| `AltoFalante`      | Reprodução de áudio      |
| `Usuario`          | Representa participantes |
| `SessaoVoIP`       | Gerencia chamadas        |

---

### Herança ("é-um")

| Relação                          |
| -------------------------------- |
| `Microfone → DispositivoAudio`   |
| `AltoFalante → DispositivoAudio` |

---

### Agregação ("tem-um")

| Relação                             |
| ----------------------------------- |
| `Usuario possui Microfone`          |
| `Usuario possui AltoFalante`        |
| `SessaoVoIP possui Usuario origem`  |
| `SessaoVoIP possui Usuario destino` |

---

## Métodos Remotos

```cpp
iniciarChamada(origem_dict, destino_dict)

encerrarChamada(sessaoId)

consultarParticipantes(sessaoId)

pingServidor()
```

---

## 4. Representação Externa de Dados e Passagem de Parâmetros

A serialização das mensagens do protocolo RMI foi implementada utilizando a biblioteca nativa `json`.

Os objetos são convertidos em dicionários (`to_dict()`) antes da serialização para transmissão.

Estrutura transportada:

```text
messageType
requestId
objectReference
methodId
arguments
```

---

## 5. Justificativa Técnica

<details>
<summary>Ver explicação detalhada</summary>

### 1. Sockets na Construção do Middleware

A exigência do trabalho busca ocultar detalhes de rede da camada de aplicação.

Como o middleware RMI foi construído manualmente, a utilização de sockets torna-se necessária para transportar mensagens serializadas entre os processos.

A aplicação em si não manipula sockets diretamente; ela apenas invoca métodos remotos.

---

### 2. Sockets no Plano de Dados (Streaming de Áudio)

Foi adotada uma arquitetura híbrida:

| Plano    | Tecnologia     |
| -------- | -------------- |
| Controle | Middleware RMI |
| Dados    | UDP Multicast  |

---

#### Justificativa

RMI opera de forma síncrona e bloqueante.

O envio contínuo de buffers de áudio por chamadas remotas sucessivas introduziria:

* aumento de latência
* overhead de serialização
* jitter elevado
* degradação do tempo real

Por esse motivo, a mídia utiliza **UDP Multicast**, enquanto a sinalização permanece sob responsabilidade do middleware RMI.

</details>

</details>

---

##  Execução

### Dependências

```bash
sudo apt install portaudio19-dev
```

### Compilação

```bash
g++ servidor.cpp -o servidor -lportaudio

g++ cliente.cpp -o cliente -lportaudio
```

### Execução

```bash
./servidor

./cliente
```
