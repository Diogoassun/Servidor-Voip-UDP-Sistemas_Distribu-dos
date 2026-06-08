from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import uvicorn

app = FastAPI(title="Servidor de Sinalização VoIP - API REST")

# Substitui o empacotamento manual
class UsuarioModel(BaseModel):
    nome: str
    ip: str

class ChamadaRequest(BaseModel):
    origem: UsuarioModel
    destino: UsuarioModel

class SessaoVoIP:
    def __init__(self, sessaoId, origem, destino):
        self.sessaoId = sessaoId
        self.origem = origem
        self.destino = destino
        self.ip_multicast = "239.0.0.1"
        self.porta_audio = 5061
        self.ativa = True

sessoes_ativas = {}

# --- ENDPOINTS DA API Substitui o RMI

@app.get("/ping")
def ping():
    return {"message": "Servidor API Operante!"}

@app.post("/chamadas")
def iniciar_chamada(payload: ChamadaRequest):
    origem = payload.origem
    destino = payload.destino
    sessaoId = f"SESSAO_{origem.nome}_{destino.nome}"
    
    sessoes_ativas[sessaoId] = SessaoVoIP(sessaoId, origem, destino)
    
    print(f"\n[API] Chamada autorizada via POST: {sessaoId}")
    print(f" Participantes: {origem.nome} -> {destino.nome}")
    
    return {
        "status": "OK",
        "sessaoId": sessaoId,
        "ip_multicast": sessoes_ativas[sessaoId].ip_multicast,
        "porta_audio": sessoes_ativas[sessaoId].porta_audio
    }

@app.delete("/chamadas/{sessaoId}")
def encerrar_chamada(sessaoId: str):
    if sessaoId in sessoes_ativas:
        sessoes_ativas[sessaoId].ativa = False
        del sessoes_ativas[sessaoId]
        print(f"[API] Chamada encerrada via DELETE: {sessaoId}")
        return {"status": "Encerrada"}
    
    raise HTTPException(status_code=404, detail="Sessão não encontrada")

@app.get("/chamadas/{sessaoId}/participantes")
def consultar_participantes(sessaoId: str):
    if sessaoId in sessoes_ativas:
        s = sessoes_ativas[sessaoId]
        return {"origem": s.origem.nome, "destino": s.destino.nome, "ativa": s.ativa}
    raise HTTPException(status_code=404, detail="Sessão não encontrada")

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=5060)
