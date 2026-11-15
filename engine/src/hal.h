// Camada de abstração de hardware (HAL) do Codex.
//
// O motor e o DSP só conhecem estas interfaces. Cada alvo (Mac, GP-5, UNO Q)
// fornece a sua implementação:
//   - mac: áudio por CoreAudio (miniaudio) e controle por WebSocket;
//   - embarcado: áudio pelo codec via I2S/DMA e controle por footswitches/knobs.
// Trocar de alvo é trocar o que implementa estas interfaces, sem tocar no DSP.
#pragma once
#include <cstdint>
#include <functional>
#include <string>

namespace codex {

// Preenche `out[0..frames)` e `out[1..frames)` a partir de `in` (entrelaçado por canal,
// não-entrelaçado aqui: ponteiros separados). Chamado na thread de áudio em tempo real:
// nada de alocar memória, travar mutex ou fazer E/S aqui.
using AudioCallback = std::function<void(const float** in, float** out,
                                         uint32_t frames, uint32_t channels)>;

class AudioBackend {
public:
    virtual ~AudioBackend() = default;
    virtual bool start(AudioCallback cb) = 0;
    virtual void stop() = 0;
    virtual double sampleRate() const = 0;
    virtual std::string description() const = 0;
};

// Mensagens de controle vindas de fora (simulador, footswitches...) chegam como
// texto (JSON). O motor registra um handler. A resposta/estado é empurrada por push().
class ControlServer {
public:
    virtual ~ControlServer() = default;
    using MessageHandler = std::function<void(const std::string& json)>;
    virtual bool start(MessageHandler onMessage) = 0;
    virtual void stop() = 0;
    virtual void push(const std::string& json) = 0;  // envia estado a todos os clientes
};

}  // namespace codex
