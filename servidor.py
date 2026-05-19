import socket
import json

# --- ENTIDADES, HERANÇAS E AGREGAÇÕES ---
class DispositivoAudio:
    def __init__(self, id_disp, status="Desligado"):
        self.id_disp = id_disp
        self.status = status
    def to_dict(self): return {"id_disp": self.id_disp, "status": self.status}

class Microfone(DispositivoAudio):
    def __init__(self, id_disp):
        super().__init__(id_disp)
        self.tipo = "Microfone"
    def to_dict(self):
        d = super().to_dict(); d.update({"tipo": self.tipo}); return d

class AltoFalante(DispositivoAudio):
    def __init__(self, id_disp):
        super().__init__(id_disp)
        self.tipo = "AltoFalante"
    def to_dict(self):
        d = super().to_dict(); d.update({"tipo": self.tipo}); return d

class Usuario:
    def __init__(self, nome, ip):
        self.nome = nome
        self.ip = ip
        self.microfone = Microfone(f"mic_{nome}")
        self.altofalante = AltoFalante(f"spk_{nome}")
    def to_dict(self):
        return {"nome": self.nome, "ip": self.ip, "microfone": self.microfone.to_dict(), "altofalante": self.altofalante.to_dict()}

class SessaoVoIP:
    def __init__(self, sessaoId, origem: Usuario, destino: Usuario):
        self.sessaoId = sessaoId
        self.origem = origem
        self.destino = destino
        self.ip_multicast = "239.0.0.1"
        self.porta_audio = 5061
        self.ativa = True

# --- MIDDLEWARE RMI ---
class RMIServerMiddleware:
    def __init__(self, porta=5060):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('', porta))
        
    def getRequest(self):
        data, client_addr = self.sock.recvfrom(65535)
        return json.loads(data.decode('utf-8')), client_addr

    def sendReply(self, reply_data, clientHost, clientPort):
        resposta_json = json.dumps(reply_data).encode('utf-8')
        self.sock.sendto(resposta_json, (clientHost, clientPort))

class ServidorVoIPRemoto:
    def __init__(self):
        self.sessoes = {}

    def iniciarChamada(self, origem_dict, destino_dict):
        origem = Usuario(origem_dict['nome'], origem_dict['ip'])
        destino = Usuario(destino_dict['nome'], destino_dict['ip'])
        sessaoId = f"SESSAO_{origem.nome}_{destino.nome}"
        self.sessoes[sessaoId] = SessaoVoIP(sessaoId, origem, destino)
        
        print(f" Participantes: {origem.nome} conectou com {destino.nome} (IP: {origem.ip})")

        print(f"\n[RMI] Chamada autorizada: {sessaoId}")
        return {
            "status": "OK", "sessaoId": sessaoId, 
            "ip_multicast": self.sessoes[sessaoId].ip_multicast,
            "porta_audio": self.sessoes[sessaoId].porta_audio
        }

    def encerrarChamada(self, sessaoId):
        if sessaoId in self.sessoes:
            self.sessoes[sessaoId].ativa = False
            print(f"[RMI] Chamada encerrada: {sessaoId}")
            return {"status": "Encerrada"}
        return {"status": "Erro"}

    def consultarParticipantes(self, sessaoId):
        if sessaoId in self.sessoes:
            s = self.sessoes[sessaoId]
            return f"Origem: {s.origem.nome} | Destino: {s.destino.nome}"
        return "Sessão não encontrada"

    def pingServidor(self):
        return "Servidor RMI Operante!"

def main():
    middleware = RMIServerMiddleware(porta=5060)
    servico = ServidorVoIPRemoto()
    print("--- SERVIDOR DE SINALIZAÇÃO RMI ATIVO ---")
    while True:
        try:
            req, addr = middleware.getRequest()
            if req.get("messageType") == 0:
                methodId = req.get("methodId")
                args = req.get("arguments")
                resposta = None
                
                if methodId == "iniciarChamada": resposta = servico.iniciarChamada(args[0], args[1])
                elif methodId == "encerrarChamada": resposta = servico.encerrarChamada(args[0])
                elif methodId == "consultarParticipantes": resposta = servico.consultarParticipantes(args[0])
                elif methodId == "pingServidor": resposta = servico.pingServidor()

                middleware.sendReply({"messageType": 1, "requestId": req.get("requestId"), "result": resposta}, addr[0], addr[1])
        except KeyboardInterrupt:
            break

if __name__ == "__main__":
    main()
