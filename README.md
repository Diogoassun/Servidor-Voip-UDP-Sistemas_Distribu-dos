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
