#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <portaudio.h>

// Constantes unificadas (devem ser iguais no cliente!)
#define PORTA_SINALIZACAO 5060
#define FRAMES_PER_BUFFER 960
#define SAMPLE_RATE 48000

using namespace std;

// --- POJOs ---
class Chamada {
   private:
    int id_call;
    string origin;
    string destination;
    long timestamp_start;
    unsigned char status;
    int audio_port;

   public:
    Chamada() : id_call(0), timestamp_start(0), status(0), audio_port(0) {}
    Chamada(int id, string origem, string destino, long timestamp, unsigned char stats, int porta_audio) {
        id_call = id; origin = origem; destination = destino;
        timestamp_start = timestamp; status = stats; audio_port = porta_audio;
    }
    void setId_call(int id) { id_call = id; }
    int getId_call() { return id_call; }
    void setOrigin(string origem) { origin = origem; }
    string getOrigin() { return origin; }
    void setDestination(string destino) { destination = destino; }
    string getDestination() { return destination; }
    void setTimestamp_start(long timestamp) { timestamp_start = timestamp; }
    long getTimestamp_start() { return timestamp_start; }
    void setStatus(unsigned char stats) { status = stats; }
    unsigned char getStatus() { return status; }
    void setAudio_port(int porta_audio) { audio_port = porta_audio; }
    int getAudio_port() { return audio_port; }
};

// --- MARSHALLING ---
class calloutputstream {
   private:
    ostream& destino;
    Chamada* listaChamadas;
    int qtdObjetos;
   public:
    calloutputstream(Chamada* lista, int quantidade, ostream& out)
        : listaChamadas(lista), qtdObjetos(quantidade), destino(out) {}
    void writeAll() {
        for (int i = 0; i < qtdObjetos; i++) {
            int id = listaChamadas[i].getId_call();
            long ts = listaChamadas[i].getTimestamp_start();
            string org = listaChamadas[i].getOrigin();
            int tamOrg = org.size();
            string dst = listaChamadas[i].getDestination();
            int tamDst = dst.size();
            unsigned char st = listaChamadas[i].getStatus();
            int porta = listaChamadas[i].getAudio_port();
            destino.write(reinterpret_cast<const char*>(&id), sizeof(id));
            destino.write(reinterpret_cast<const char*>(&ts), sizeof(ts));
            destino.write(reinterpret_cast<const char*>(&tamOrg), sizeof(tamOrg));
            destino.write(org.c_str(), tamOrg);
            destino.write(reinterpret_cast<const char*>(&tamDst), sizeof(tamDst));
            destino.write(dst.c_str(), tamDst);
            destino.write(reinterpret_cast<const char*>(&st), sizeof(st));
            destino.write(reinterpret_cast<const char*>(&porta), sizeof(porta));
        }
    }
};

class callinputstream {
   private:
    istream& origem;
    Chamada* listaChamadas;
    int qtdObjetos;
   public:
    callinputstream(Chamada* lista, int quantidade, istream& in)
        : listaChamadas(lista), qtdObjetos(quantidade), origem(in) {}
    void readAll() {
        for (int i = 0; i < qtdObjetos; i++) {
            int id, tamOrg, tamDst, porta;
            long ts;
            unsigned char st;
            origem.read(reinterpret_cast<char*>(&id), sizeof(id));
            origem.read(reinterpret_cast<char*>(&ts), sizeof(ts));
            origem.read(reinterpret_cast<char*>(&tamOrg), sizeof(tamOrg));
            string s_org; s_org.resize(tamOrg);
            origem.read(&s_org[0], tamOrg);
            origem.read(reinterpret_cast<char*>(&tamDst), sizeof(tamDst));
            string s_dst; s_dst.resize(tamDst);
            origem.read(&s_dst[0], tamDst);
            origem.read(reinterpret_cast<char*>(&st), sizeof(st));
            origem.read(reinterpret_cast<char*>(&porta), sizeof(porta));
            listaChamadas[i].setId_call(id);
            listaChamadas[i].setTimestamp_start(ts);
            listaChamadas[i].setOrigin(s_org);
            listaChamadas[i].setDestination(s_dst);
            listaChamadas[i].setStatus(st);
            listaChamadas[i].setAudio_port(porta);
        }
    }
};

int main() {
    // --- PORTAUDIO ---
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        cerr << "Erro ao inicializar PortAudio: " << Pa_GetErrorText(err) << endl;
        return 1;
    }

    PaStream *stream;
    err = Pa_OpenDefaultStream(&stream, 0, 1, paInt16, SAMPLE_RATE, FRAMES_PER_BUFFER, NULL, NULL);
    if (err != paNoError) {
        cerr << "Erro ao abrir stream: " << Pa_GetErrorText(err) << endl;
        Pa_Terminate();
        return 1;
    }

    // --- SOCKET ---
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) { perror("Erro socket"); Pa_Terminate(); return 1; }

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // FIX: define um timeout para o recvfrom não bloquear indefinidamente no loop de áudio
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORTA_SINALIZACAO);

    if (bind(s, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Erro bind");
        close(s);
        Pa_Terminate();
        return 1;
    }

    cout << "--- SERVIDOR VOIP ATIVO ---" << endl;
    cout << "Escutando na porta: " << PORTA_SINALIZACAO << endl;

    while (true) {
        char buffer[2048];

        // FIX: clilen DEVE ser reinicializado antes de cada recvfrom.
        // Sem isso, após a primeira chamada o valor fica corrompido e
        // os pacotes seguintes podem vir com endereço inválido.
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);

        int n = recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, &clilen);

        if (n > 0) {
            // Unmarshalling da sinalização
            stringstream ss_in(string(buffer, n));
            Chamada recebida;
            callinputstream inStream(&recebida, 1, ss_in);
            inStream.readAll();

            cout << "\n[SINALIZAÇÃO] INVITE de: " << recebida.getOrigin() << endl;

            // Aceitar chamada (status 3)
            recebida.setStatus(3);
            recebida.setAudio_port(PORTA_SINALIZACAO);

            stringstream ss_out;
            calloutputstream outStream(&recebida, 1, ss_out);
            outStream.writeAll();
            string resposta = ss_out.str();

            sendto(s, resposta.c_str(), resposta.size(), 0, (struct sockaddr *)&cli_addr, clilen);
            cout << "[RESPOSTA] OK enviado. Ligando os alto-falantes..." << endl;

            Pa_StartStream(stream);

            // --- LOOP DE ÁUDIO ---
            short audioBuffer[FRAMES_PER_BUFFER];
            int pacotesRecebidos = 0;
            bool conversaAtiva = true;

            while (conversaAtiva) {
                // FIX: clilen deve ser reinicializado a cada iteração do loop de áudio
                // para garantir que o endereço do cliente seja lido corretamente.
                socklen_t audioCliLen = sizeof(cli_addr);

                int bytesAudio = recvfrom(s, (char*)audioBuffer, sizeof(audioBuffer), 0,
                                          (struct sockaddr *)&cli_addr, &audioCliLen);

                if (bytesAudio > 0) {
                    // Verifica se recebeu frames suficientes antes de escrever
                    int framesRecebidos = bytesAudio / sizeof(short);
                    if (framesRecebidos >= FRAMES_PER_BUFFER) {
                        PaError writeErr = Pa_WriteStream(stream, audioBuffer, FRAMES_PER_BUFFER);
                        if (writeErr != paNoError && writeErr != paOutputUnderflowed) {
                            cerr << "[AUDIO] Erro Pa_WriteStream: " << Pa_GetErrorText(writeErr) << endl;
                        }
                    }

                    pacotesRecebidos++;
                    if (pacotesRecebidos % 100 == 0) {
                        cout << "[AUDIO] Streaming ativo de " << recebida.getOrigin() << "..." << endl;
                    }
                } else {
                    // bytesAudio <= 0: timeout ou erro — encerra a chamada
                    conversaAtiva = false;
                }
            }

            Pa_StopStream(stream);
            cout << "[INFO] Chamada encerrada para " << recebida.getOrigin() << endl;
        }
    }

    Pa_CloseStream(stream);
    Pa_Terminate();
    close(s);
    return 0;
}
