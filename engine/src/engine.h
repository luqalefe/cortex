// Núcleo portátil do Codex: liga o DSP aos parâmetros e ao roteamento de áudio.
// Não conhece CoreAudio nem WebSocket — só as interfaces da HAL. É este código
// que, no futuro, roda igual no pedal real.
#pragma once
#include <atomic>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "dsp_codex.h"
#include "params.h"

namespace codex {

class Engine {
public:
    static constexpr int kNumParams = static_cast<int>(kParams.size());

    void init(double sampleRate) {
        sampleRate_ = sampleRate;
        dsp_.init(sampleRate);
        for (int i = 0; i < kNumParams; ++i) params_[i].store(kParams[i].def);
        bypass_.store(false);
        level_.store(0.0f);
        monoIn_.assign(kMaxBlock, 0.0f);
        monoOut_.assign(kMaxBlock, 0.0f);
    }

    double sampleRate() const { return sampleRate_; }

    // --- controle (thread de controle) ---
    bool setParam(const std::string& id, float value) {
        int idx = paramIndex(id);
        if (idx < 0) return false;
        const auto& p = kParams[idx];
        if (value < p.min) value = p.min;
        if (value > p.max) value = p.max;
        params_[idx].store(value);
        return true;
    }
    void setBypass(bool b) { bypass_.store(b); }
    void toggleBypass() { bypass_.store(!bypass_.load()); }

    // --- áudio (thread de tempo real) ---
    // Mistura a entrada para mono, processa o delay e devolve igual nos canais de saída.
    void process(const float** in, float** out, uint32_t frames, uint32_t channels) {
        uint32_t n = frames > kMaxBlock ? kMaxBlock : frames;
        // entrada -> mono
        for (uint32_t i = 0; i < n; ++i) {
            float s = 0.0f;
            for (uint32_t c = 0; c < channels; ++c) s += in[c] ? in[c][i] : 0.0f;
            monoIn_[i] = channels ? s / static_cast<float>(channels) : 0.0f;
        }
        if (bypass_.load()) {
            std::copy(monoIn_.begin(), monoIn_.begin() + n, monoOut_.begin());
        } else {
            for (int i = 0; i < kNumParams; ++i) dsp_.setParam(kParams[i].id, params_[i].load());
            float* pin[1] = {monoIn_.data()};
            float* pout[1] = {monoOut_.data()};
            dsp_.process(static_cast<int>(n), pin, pout);
        }
        // mono -> todos os canais de saída + medidor de pico
        float peak = 0.0f;
        for (uint32_t i = 0; i < n; ++i) {
            float s = monoOut_[i];
            float a = std::fabs(s);
            if (a > peak) peak = a;
            for (uint32_t c = 0; c < channels; ++c)
                if (out[c]) out[c][i] = s;
        }
        // decaimento suave do medidor, para a "LED" não piscar bruto
        float prev = level_.load();
        level_.store(peak > prev ? peak : prev * 0.85f);
    }

    // --- estado para o simulador ---
    std::string snapshotJson() {
        char buf[320];
        std::snprintf(buf, sizeof(buf),
            "{\"type\":\"state\",\"bypass\":%s,\"level\":%.4f,\"sr\":%.0f,"
            "\"params\":{\"time\":%.2f,\"repeats\":%.3f,\"mix\":%.3f,\"tone\":%.1f}}",
            bypass_.load() ? "true" : "false", level_.load(), sampleRate_,
            params_[0].load(), params_[1].load(), params_[2].load(), params_[3].load());
        return buf;
    }

private:
    static constexpr uint32_t kMaxBlock = 8192;
    CodexDsp dsp_;
    double sampleRate_ = 44100.0;
    std::atomic<float> params_[kNumParams];
    std::atomic<bool> bypass_{false};
    std::atomic<float> level_{0.0f};
    std::vector<float> monoIn_, monoOut_;
};

}  // namespace codex
