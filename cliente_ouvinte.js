const dgram = require('dgram');
const http = require('http');
const process = require('process');
const Speaker = require('speaker');

async function iniciarChamadaOuvinte() {
    console.log("--- CLIENTE OUVINTE NODE.JS (ÁUDIO REAL + FIFO) ---");

    const payload = JSON.stringify({
        origem: { nome: "Leandro_Node", ip: "127.0.0.1" },
        destino: { nome: "Diogo_CPP", ip: "127.0.0.1" }
    });

    const opcoesPost = {
        hostname: '127.0.0.1',
        port: 5060,
        path: '/chamadas',
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'Content-Length': Buffer.byteLength(payload)
        }
    };

    const req = http.request(opcoesPost, (res) => {
        let data = '';
        res.on('data', (chunk) => { data += chunk; });
        
        res.on('end', () => {
            const resposta = JSON.parse(data);

            if (resposta.status === "OK") {
                console.log(`Sinalização Aprovada! Grupo Multicast: ${resposta.ip_multicast}`);

                // Configura o Alto-falante nativo do sistema
                const altoFalante = new Speaker({
                    channels: 1,
                    bitDepth: 16,
                    sampleRate: 48000
                });

                const servidorUDP = dgram.createSocket({ type: 'udp4', reuseAddr: true });
                let proxima_sequencia_esperada = 1;

                servidorUDP.on('message', (msg) => {
                    if (msg.length < 4) return;

                    // Extrai os primeiros 4 bytes do pacote 
                    const seq_recebida = msg.readUInt32BE(0);

                    const dados_audio = msg.subarray(4);

                    // Algoritmo de Controle FIFO
                    if (seq_recebida === proxima_sequencia_esperada) {
                        altoFalante.write(dados_audio);
                        proxima_sequencia_esperada++;
                    } else if (seq_recebida > proxima_sequencia_esperada) {
                        console.log(`\n[FIFO GAP] Pacotes perdidos! Saltou para #${seq_recebida}`);
                        altoFalante.write(dados_audio);
                        proxima_sequencia_esperada = seq_recebida + 1;
                    } else {
                        console.log(`\n[FIFO REJEITADO] Pacote atrasado/duplicado descartado: #${seq_recebida}`);
                    }
                });

                servidorUDP.bind(resposta.porta_audio, () => {
                    servidorUDP.addMembership(resposta.ip_multicast);
                    console.log("[SOCKET] Ouvindo áudio real do grupo com checagem FIFO ativa...");
                });

                process.on('SIGINT', () => {
                    servidorUDP.close();
                    altoFalante.end();
                    process.exit();
                });
            }
        });
    });
    req.write(payload);
    req.end();
}

iniciarChamadaOuvinte();
