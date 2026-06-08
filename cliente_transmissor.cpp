#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <curl/curl.h>
#include <portaudio.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#define SAMPLE_RATE 48000
#define FRAMES_PER_BUFFER 960
#define NUM_CHANNELS 1

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    s->append((char*)contents, newLength);
    return newLength;
}

void transmitir_audio_real_fifo(std::string ip_multicast, int porta) {
    // Configura Socket UDP Multicast
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    int ttl = 5;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    struct sockaddr_in addrDest;
    std::memset(&addrDest, 0, sizeof(addrDest));
    addrDest.sin_family = AF_INET;
    addrDest.sin_port = htons(porta);
    addrDest.sin_addr.s_addr = inet_addr(ip_multicast.c_str());

    // Inicializa Microfone
    Pa_Initialize();
    PaStream* stream;
    Pa_OpenDefaultStream(&stream, NUM_CHANNELS, 0, paInt16, SAMPLE_RATE, FRAMES_PER_BUFFER, NULL, NULL);
    Pa_StartStream(stream);

    std::cout << "\n[MICROFONE C++] Transmitindo ÁUDIO REAL + GARANTIA FIFO...\n";
    
    uint32_t numero_sequencia = 1;
    short buffer_audio[FRAMES_PER_BUFFER * NUM_CHANNELS];
    
    // Tamanho do buffer de áudio em bytes + 4 bytes do cabeçalho de sequência
    size_t tamanho_audio_bytes = sizeof(buffer_audio);
    size_t tamanho_total_pacote = 4 + tamanho_audio_bytes;
    char* pacote_final = new char[tamanho_total_pacote];

    while (true) {
        Pa_ReadStream(stream, buffer_audio, FRAMES_PER_BUFFER);

        uint32_t seq_network_order = htonl(numero_sequencia);
        std::memcpy(pacote_final, &seq_network_order, 4);

        std::memcpy(pacote_final + 4, buffer_audio, tamanho_audio_bytes);

        sendto(sock, pacote_final, tamanho_total_pacote, 0, (struct sockaddr*)&addrDest, sizeof(addrDest));

        std::cout << " Enviando pacote de áudio real | Sequência FIFO: #" << numero_sequencia << " \r" << std::flush;
        numero_sequencia++;
    }

    delete[] pacote_final;
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    close(sock);
}

int main() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        json payload = {
            {"origem", {{"nome", "Diogo_CPP"}, {"ip", "127.0.0.1"}}},
            {"destino", {{"nome", "Leandro_Node"}, {"ip", "127.0.0.1"}}}
        };
        std::string json_str = payload.dump();

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:5060/chamadas");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if(res == CURLE_OK) {
            auto resposta = json::parse(readBuffer);
            if (resposta["status"] == "OK") {
                transmitir_audio_real_fifo(resposta["ip_multicast"], resposta["porta_audio"]);
            }
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
}
