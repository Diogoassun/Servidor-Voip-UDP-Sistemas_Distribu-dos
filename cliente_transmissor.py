import socket
import json
import threading
import pyaudio
import audioop
import time

audio_interface = pyaudio.PyAudio()
ativo = True

class Microfone:
    def __init__(self, nome_dono):
        self.nome_dono = nome_dono

    def ligar_multicast(self, ip_multicast, porta):
        def thread_captura():
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 5)
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_LOOP, 1)
            
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton('0.0.0.0'))

            stream = audio_interface.open(format=pyaudio.paInt16, channels=1, rate=48000, input=True, frames_per_buffer=960)
            print("\n[MICROFONE] Transmitindo...")

            while ativo:
                try:
                    data = stream.read(960, exception_on_overflow=False)
                    volume = audioop.rms(data, 2)
                    
                    if volume < 15:
                        print(f" MAC BLOQUEOU O MIC (Vol: {volume:04d}) - Libere nos Ajustes!   ", end='\r')
                    else:
                        barra = "█" * min(int(volume / 100), 20)
                        print(f" Enviando: {volume:04d} |{barra:20s}|", end='\r')
                        
                    sock.sendto(data, (ip_multicast, porta))
                except OSError: break
            
            stream.stop_stream(); stream.close(); sock.close()

        threading.Thread(target=thread_captura, daemon=True).start()

class UsuarioTransmissor:
    def __init__(self, nome, ip):
        self.nome = nome
        self.ip = ip
        self.microfone = Microfone(nome)
    def to_dict(self): return {"nome": self.nome, "ip": self.ip}

class RMIClientMiddleware:
    def __init__(self, server_ip, server_port):
        self.server_ip = server_ip
        self.server_port = server_port
        self.req_id_counter = 0

    def doOperation(self, objectReference, methodId, arguments):
        self.req_id_counter += 1
        mensagem = {"messageType": 0, "requestId": self.req_id_counter, "objectReference": objectReference, "methodId": methodId, "arguments": arguments}
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(2.0)
        try:
            sock.sendto(json.dumps(mensagem).encode('utf-8'), (self.server_ip, self.server_port))
            data, _ = sock.recvfrom(65535)
            reply = json.loads(data.decode('utf-8'))
            if reply.get("messageType") == 1: return reply.get("result")
        except socket.timeout: return {"status": "Erro de Timeout"}
        finally: sock.close()

def main():
    global ativo
    rmi = RMIClientMiddleware("127.0.0.1", 5060)
    print("--- CLIENTE TRANSMISSOR (RMI) ---")
    meu_usuario = UsuarioTransmissor("Diogo", "127.0.0.1")
    destino = UsuarioTransmissor("Leandro", "127.0.0.1")

    resposta = rmi.doOperation("ServidorVoIP", "iniciarChamada", [meu_usuario.to_dict(), destino.to_dict()])
    
    if resposta and resposta.get("status") == "OK":
        meu_usuario.microfone.ligar_multicast(resposta.get("ip_multicast"), resposta.get("porta_audio"))
        print(" Modo Transmissor ativo. Pressione Ctrl+C para desligar.")
        try:
            while True: time.sleep(1)
        except KeyboardInterrupt:
            ativo = False
            rmi.doOperation("ServidorVoIP", "encerrarChamada", [resposta.get("sessaoId")])
            audio_interface.terminate()

if __name__ == "__main__":
    main()
