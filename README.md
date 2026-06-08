# VoIP com Sinalização REST & Streaming Multicast FIFO

Sistema distribuído híbrido para comunicação de voz sobre IP (VoIP) em tempo real utilizando **Python (FastAPI)**, **C++**, **Node.js** e **UDP Multicast**.

O projeto foi desenvolvido com foco em:

* Desacoplamento espacial entre participantes
* Interoperabilidade entre múltiplas linguagens
* Separação entre plano de controle e plano de dados
* Comunicação multicast escalável
* Garantia de ordenação FIFO na camada de aplicação

---

# Visão Geral

A arquitetura é dividida em dois planos independentes:

## 1. Plano de Sinalização (Controle)

Servidor REST desenvolvido em Python/FastAPI responsável por:

* Registro de participantes
* Criação de chamadas
* Orquestração da sessão
* Descoberta dos parâmetros de comunicação

Comunicação:

```text
HTTP/TCP
```

---

## 2. Plano de Mídia (Dados)

Fluxo contínuo de áudio transmitido diretamente entre os clientes utilizando:

```text
UDP Multicast
```

O áudio não passa pelo servidor central.

---

# Arquitetura

```text
              +-----------------------------------+
              |   SERVIDOR DE SINALIZAÇÃO (API)   |
              |          Python / FastAPI         |
              +-----------------------------------+
                 ^                             ^
                 | (POST /chamadas)            | (POST /chamadas)
                 | HTTP/TCP                    | HTTP/TCP
                 v                             v

+--------------------------------+             +----------------------------------+
|  CLIENTE TRANSMISSOR (C++)     |             |    CLIENTE OUVINTE (Node.js)     |
|                                |             |                                  |
|  - Captura Mic via PortAudio   |             |  - Reprodução via Speaker        |
|  - Injeta Sequência FIFO       |             |  - Validação FIFO                |
+--------------------------------+             +----------------------------------+

                    ^
                    |
                    |
+---------------------------------------------------------------+
|            UDP MULTICAST (239.0.0.1)                          |
|                                                               |
|             Áudio PCM + Cabeçalho FIFO                        |
+---------------------------------------------------------------+
```

---

# Protocolo FIFO na Camada de Aplicação

O transporte UDP não oferece:

* Garantia de entrega
* Garantia de ordenação
* Garantia contra duplicação

Para resolver isso foi criado um cabeçalho de aplicação de 4 bytes.

---

## Estrutura do Pacote

```text
Posição:

[ Byte 0 ] [ Byte 1 ] [ Byte 2 ] [ Byte 3 ]
[ Byte 4 ................................ Byte 1923 ]

+-----------------------------------------+
|      Número de Sequência FIFO           |
|      (uint32_t / 4 bytes)               |
+-----------------------------------------+

+-----------------------------------------+
|          Dados de Áudio PCM             |
|        16 bits / 1920 bytes             |
+-----------------------------------------+
```

---

## Estrutura Lógica

```text
+=========================================+
|          CÓDIGO DE CONTROLE             |
+=========================================+

+=========================================+
|               PAYLOAD                   |
+=========================================+
```

---

# Funcionamento do Transmissor (C++)

Cada pacote recebe um identificador sequencial.

Exemplo:

```cpp
std::memcpy(pacote, &seq_network, 4);
```

O pacote é então enviado ao grupo multicast:

```text
239.0.0.1
```

---

# Funcionamento do Receptor (Node.js)

Ao receber um pacote:

```javascript
const seq = msg.readUInt32BE(0);
```

O número de sequência é comparado com o valor esperado.

---

## Caso 1 — Pacote Correto

```text
seq_recebida == seq_esperada
```

Resultado:

```text
[FIFO OK]
```

O áudio é entregue imediatamente ao dispositivo de saída.

```javascript
altoFalante.write(audioBuffer);
```

---

## Caso 2 — Perda de Pacotes

```text
seq_recebida > seq_esperada
```

Resultado:

```text
[FIFO GAP]
```

O receptor detecta perda física de pacotes.

Para preservar o comportamento em tempo real, o fluxo continua a partir do pacote mais recente.

---

## Caso 3 — Pacote Atrasado ou Duplicado

```text
seq_recebida < seq_esperada
```

Resultado:

```text
[FIFO REJEITADO]
```

O pacote é descartado imediatamente.

Isso evita:

* Eco
* Repetição de áudio
* Distorções
* Artefatos acústicos

---

# Propriedades de Comunicação Indireta

## Desacoplamento Espacial

O transmissor não conhece:

* Endereço IP dos ouvintes
* Quantidade de ouvintes
* Localização dos ouvintes

Ele apenas envia para:

```text
239.0.0.1
```

Qualquer novo cliente que execute:

```javascript
socket.addMembership("239.0.0.1");
```

passará a receber o fluxo instantaneamente.

---

## Isolamento do Servidor

O servidor REST nunca manipula os dados de áudio.

Sua responsabilidade limita-se à sinalização.

Consequentemente:

* Menor carga computacional
* Menor uso de banda
* Escalabilidade superior
* Separação clara de responsabilidades

---

# Pré-requisitos

## macOS

Instalar PortAudio:

```bash
brew install portaudio
```

---

## Python (Servidor)

Instalar dependências:

```bash
pip install fastapi uvicorn pydantic
```

---

## Node.js (Ouvinte)

Instalar dependências:

```bash
npm install speaker
```

---

# Execução do Sistema

Abra três terminais.

---

## Iniciar Servidor de Sinalização

```bash
python3 servidor.py
```

---

## Iniciar Cliente Ouvinte

```bash
node cliente_ouvinte.js
```

---

## Compilar Cliente Transmissor

```bash
g++ -std=c++11 cliente_transmissor.cpp \
-o cliente_transmissor \
-I/opt/homebrew/include \
-L/opt/homebrew/lib \
-lcurl \
-lportaudio
```

---

## Executar o Transmissor

```bash
./cliente_transmissor
```

---

# Exemplo de Logs

```text
[FIFO OK] Pacote 120
[FIFO OK] Pacote 121
[FIFO OK] Pacote 122

[FIFO GAP] Esperado 123 Recebido 126

[FIFO OK] Pacote 127

[FIFO REJEITADO] Pacote atrasado 124
```

---

# Objetivos Acadêmicos Demonstrados

Este projeto demonstra na prática:

* Comunicação distribuída
* Sistemas multicast
* Comunicação indireta
* Desacoplamento espacial
* Protocolos de aplicação
* VoIP em tempo real
* Tolerância a perdas UDP
* Garantia FIFO em camada de aplicação
* Interoperabilidade entre C++, Python e Node.js

---

# Tecnologias Utilizadas

| Tecnologia    | Função                         |
| ------------- | ------------------------------ |
| C++           | Captura e transmissão de áudio |
| PortAudio     | Interface com microfone        |
| Node.js       | Recepção e reprodução          |
| Speaker       | Saída de áudio                 |
| Python        | Sinalização                    |
| FastAPI       | API REST                       |
| UDP Multicast | Transporte de mídia            |
| HTTP/TCP      | Controle da sessão             |

---

# Autor

Projeto acadêmico de Sistemas Distribuídos demonstrando a separação entre plano de controle e plano de dados em um sistema VoIP multicast com garantia FIFO implementada na camada de aplicação.
