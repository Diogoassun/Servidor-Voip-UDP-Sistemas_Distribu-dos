import socket
import struct
import json
import threading
import pyaudio
import audioop
import time

audio_interface = pyaudio.PyAudio()
ativo = True

class AltoFalante:
    def __init__(self, nome_dono):
        self.nome_dono = nome_dono

    def escutar_multicast(self, ip_multicast, porta):
        def thread_reproducao():
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            if hasattr(socket, 'SO_REUSEPORT'): sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
            sock.bind(('', porta)) 

            mreq = struct.pack("4sl", socket.inet_aton(ip_multicast), socket.INADDR_ANY)
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

            canais = 1
            try: 
                stream = audio_interface.open(format=pyaudio.paInt16, channels=1, rate=48000, output=True, frames_per_buffer=960)
            except OSError: 
                stream = audio_interface.open(format=pyaudio.paInt16, channels=2, rate=48000, output=True, frames_per_buffer=960)
                canais = 2

            print("\n[ALTO-FALANTE] Escutando a rede...")

            while ativo:
                try:
                    data, _ = sock.recvfrom(960 * 2)
                    
                    if data and len(data) % 2 == 0:
                        volume = audioop.rms(data, 2)
                        barra = "█" * min(int(volume / 100), 20)
                        print(f" Recebendo: {volume:04d} |{barra:20s}|", end='\r')
                        
                        if canais == 2: stream.write(audioop.tostereo(data, 2, 1, 1))
                        else: stream.write(data)
                except OSError: break
            
            stream.stop_stream(); stream.close(); sock.close()

        threading.Thread(target=thread_reproducao, daemon=True).start()

class UsuarioOuvinte:
    def __init__(self, nome, ip):
        self.nome = nome
        self.ip = ip
        self.altofalante = AltoFalante(nome)
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
    print("--- CLIENTE OUVINTE (RMI) ---")
    meu_usuario = UsuarioOuvinte("Ouvinte", "127.0.0.1")
    destino = UsuarioOuvinte("Transmissor", "127.0.0.1")

    resposta = rmi.doOperation("ServidorVoIP", "iniciarChamada", [meu_usuario.to_dict(), destino.to_dict()])
    
    if resposta and resposta.get("status") == "OK":
        meu_usuario.altofalante.escutar_multicast(resposta.get("ip_multicast"), resposta.get("porta_audio"))
        print(" Modo Ouvinte ativo. Pressione Ctrl+C para desligar.")
        try:
            while True: time.sleep(1)
        except KeyboardInterrupt:
            ativo = False
            rmi.doOperation("ServidorVoIP", "encerrarChamada", [resposta.get("sessaoId")])
            audio_interface.terminate()

if __name__ == "__main__":
    main()
