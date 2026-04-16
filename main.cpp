#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <atomic>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <portaudio.h>

#define PORTA_SINALIZACAO 5060
#define PORTA_AUDIO       5061
#define GRUPO_MULTICAST   "239.0.0.1"
#define FRAMES_PER_BUFFER 960
#define SAMPLE_RATE       48000

using namespace std;

// --- POJO ---
class Chamada {
   private:
    int id_call; string origin; string destination;
    long timestamp_start; unsigned char status; int audio_port;
   public:
    Chamada() : id_call(0), timestamp_start(0), status(0), audio_port(0) {}
    void setId_call(int id)            { id_call = id; }
    int getId_call()                   { return id_call; }
    void setOrigin(string o)           { origin = o; }
    string getOrigin()                 { return origin; }
    void setDestination(string d)      { destination = d; }
    string getDestination()            { return destination; }
    void setTimestamp_start(long t)    { timestamp_start = t; }
    long getTimestamp_start()          { return timestamp_start; }
    void setStatus(unsigned char s)    { status = s; }
    unsigned char getStatus()          { return status; }
    void setAudio_port(int p)          { audio_port = p; }
    int getAudio_port()                { return audio_port; }
};

// --- MARSHALLING ---
class calloutputstream {
    ostream& destino; Chamada* lista; int qtd;
   public:
    calloutputstream(Chamada* l, int q, ostream& o) : lista(l), qtd(q), destino(o) {}
    void writeAll() {
        for (int i = 0; i < qtd; i++) {
            int id = lista[i].getId_call();
            long ts = lista[i].getTimestamp_start();
            string org = lista[i].getOrigin(); int tamOrg = org.size();
            string dst = lista[i].getDestination(); int tamDst = dst.size();
            unsigned char st = lista[i].getStatus();
            int porta = lista[i].getAudio_port();
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
    istream& origem; Chamada* lista; int qtd;
   public:
    callinputstream(Chamada* l, int q, istream& i) : lista(l), qtd(q), origem(i) {}
    void readAll() {
        for (int i = 0; i < qtd; i++) {
            int id, tamOrg, tamDst, porta; long ts; unsigned char st;
            origem.read(reinterpret_cast<char*>(&id), sizeof(id));
            origem.read(reinterpret_cast<char*>(&ts), sizeof(ts));
            origem.read(reinterpret_cast<char*>(&tamOrg), sizeof(tamOrg));
            string s_org; s_org.resize(tamOrg); origem.read(&s_org[0], tamOrg);
            origem.read(reinterpret_cast<char*>(&tamDst), sizeof(tamDst));
            string s_dst; s_dst.resize(tamDst); origem.read(&s_dst[0], tamDst);
            origem.read(reinterpret_cast<char*>(&st), sizeof(st));
            origem.read(reinterpret_cast<char*>(&porta), sizeof(porta));
            lista[i].setId_call(id); lista[i].setTimestamp_start(ts);
            lista[i].setOrigin(s_org); lista[i].setDestination(s_dst);
            lista[i].setStatus(st); lista[i].setAudio_port(porta);
        }
    }
};

// --- THREAD: recebe áudio do grupo multicast e reproduz ---
void threadReproducao(atomic<bool>& ativo) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("[REPROD] socket"); return; }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(PORTA_AUDIO);
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[REPROD] bind"); close(sock); return;
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(GRUPO_MULTICAST);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) < 0) {
        perror("[REPROD] IP_ADD_MEMBERSHIP"); close(sock); return;
    }

    PaStream* stream;
    PaError err = Pa_OpenDefaultStream(&stream, 0, 1, paInt16, SAMPLE_RATE, FRAMES_PER_BUFFER, NULL, NULL);
    if (err != paNoError) {
        cerr << "[REPROD] " << Pa_GetErrorText(err) << endl;
        close(sock); return;
    }
    Pa_StartStream(stream);

    short buf[FRAMES_PER_BUFFER];
    cout << "[REPROD] Ouvindo grupo multicast..." << endl;

    while (ativo) {
        struct sockaddr_in from; socklen_t len = sizeof(from);
        int bytes = recvfrom(sock, (char*)buf, sizeof(buf), 0, (struct sockaddr*)&from, &len);
        if (bytes > 0) Pa_WriteStream(stream, buf, FRAMES_PER_BUFFER);
    }

    Pa_StopStream(stream); Pa_CloseStream(stream); close(sock);
}

// --- THREAD: captura microfone e envia para o grupo multicast ---
void threadCaptura(atomic<bool>& ativo) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("[CAPTURA] socket"); return; }

    struct sockaddr_in grupoAddr;
    grupoAddr.sin_family      = AF_INET;
    grupoAddr.sin_addr.s_addr = inet_addr(GRUPO_MULTICAST);
    grupoAddr.sin_port        = htons(PORTA_AUDIO);

    unsigned char ttl = 5;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    unsigned char loop = 0;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    PaStream* stream;
    PaError err = Pa_OpenDefaultStream(&stream, 1, 0, paInt16, SAMPLE_RATE, FRAMES_PER_BUFFER, NULL, NULL);
    if (err != paNoError) {
        cerr << "[CAPTURA] " << Pa_GetErrorText(err) << endl;
        close(sock); return;
    }
    Pa_StartStream(stream);

    short buf[FRAMES_PER_BUFFER];
    cout << "[CAPTURA] Transmitindo microfone para o grupo..." << endl;

    while (ativo) {
        Pa_ReadStream(stream, buf, FRAMES_PER_BUFFER);
        sendto(sock, (char*)buf, sizeof(buf), 0, (struct sockaddr*)&grupoAddr, sizeof(grupoAddr));
    }

    Pa_StopStream(stream); Pa_CloseStream(stream); close(sock);
}

int main() {
    Pa_Initialize();

    int sSinal = socket(AF_INET, SOCK_DGRAM, 0);
    int opt = 1;
    setsockopt(sSinal, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in servAddr;
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Escuta em todas as interfaces
    servAddr.sin_port        = htons(PORTA_SINALIZACAO);
    bind(sSinal, (struct sockaddr*)&servAddr, sizeof(servAddr));

    cout << "--- SERVIDOR VOIP LOCAL ATIVO ---" << endl;
    cout << "Aguardando chamadas na porta " << PORTA_SINALIZACAO << "..." << endl;

    static atomic<bool> ativo(true);
    static bool threadsIniciadas = false;

    while (true) {
        char buffer[2048];
        struct sockaddr_in cliAddr; socklen_t clilen = sizeof(cliAddr);

        int n = recvfrom(sSinal, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliAddr, &clilen);
        if (n <= 0) continue;

        stringstream ss_in(string(buffer, n));
        Chamada recebida;
        callinputstream inStream(&recebida, 1, ss_in);
        inStream.readAll();

        cout << "[SINALIZAÇÃO] Chamada de: " << recebida.getOrigin()
             << " -> " << recebida.getDestination() << endl;

        Chamada resposta;
        resposta.setStatus(3);
        resposta.setAudio_port(PORTA_AUDIO);
        resposta.setDestination("LOCAL_GROUP");

        stringstream ss_out;
        calloutputstream outStream(&resposta, 1, ss_out);
        outStream.writeAll();
        string s_resp = ss_out.str();

        sendto(sSinal, s_resp.c_str(), s_resp.size(), 0, (struct sockaddr*)&cliAddr, clilen);
        cout << "[SINALIZAÇÃO] OK enviado para " << inet_ntoa(cliAddr.sin_addr) << endl;

        if (!threadsIniciadas) {
            thread tReprod(threadReproducao, ref(ativo));
            thread tCaptura(threadCaptura, ref(ativo));
            tReprod.detach();
            tCaptura.detach();
            threadsIniciadas = true;
        }
    }

    Pa_Terminate();
    return 0;
}   
