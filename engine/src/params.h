// Registro de parâmetros do Codex — a fonte única da verdade, compartilhada entre
// o DSP, o controle (WebSocket) e o simulador. Mantido deliberadamente portátil
// (só C++ padrão), para servir igual no Mac e no alvo embarcado.
#pragma once
#include <array>
#include <string>

namespace codex {

struct ParamDef {
    const char* id;     // nome curto, igual ao do .dsp e ao usado no WebSocket
    const char* label;  // rótulo para a interface
    const char* unit;   // "ms", "Hz", "" ...
    float min, max, def;
};

// A ordem aqui define os índices usados pelo motor (Engine::kNumParams).
inline constexpr std::array<ParamDef, 4> kParams{{
    {"time",    "Time",    "ms", 1.0f,   2000.0f,  480.0f},
    {"repeats", "Repeats", "",   0.0f,   0.95f,    0.40f},
    {"mix",     "Mix",     "",   0.0f,   1.0f,     0.35f},
    {"tone",    "Tone",    "Hz", 800.0f, 16000.0f, 6000.0f},
}};

inline int paramIndex(const std::string& id) {
    for (size_t i = 0; i < kParams.size(); ++i)
        if (id == kParams[i].id) return static_cast<int>(i);
    return -1;
}

}  // namespace codex
