// Invólucro do DSP gerado pelo Faust (classe CodexDelay).
//
// Isola o resto do motor do código gerado: expõe init/process e ajuste de
// parâmetros por nome. O DSP em si é portátil; só este arquivo conhece o Faust.
#pragma once
#include <string>
#include <vector>

// Classes-base do Faust que o código gerado espera (dsp, UI, Meta).
#include <faust/gui/meta.h>
#include <faust/dsp/dsp.h>
#include <faust/gui/UI.h>
#include <faust/gui/MapUI.h>

#include "../generated/codex_delay.hpp"  // classe CodexDelay (gerada de dsp/codex_delay.dsp)

namespace codex {

class CodexDsp {
public:
    void init(double sampleRate) {
        fDsp_.init(static_cast<int>(sampleRate));
        fDsp_.buildUserInterface(&fMap_);
    }

    int numInputs() const { return const_cast<CodexDelay&>(fDsp_).getNumInputs(); }
    int numOutputs() const { return const_cast<CodexDelay&>(fDsp_).getNumOutputs(); }

    // true se o parâmetro existe (nome curto, ex.: "mix").
    bool setParam(const std::string& id, float value) {
        auto& m = fMap_.getShortnameMap();
        auto it = m.find(id);
        if (it == m.end()) return false;
        *it->second = value;
        return true;
    }

    float getParam(const std::string& id) {
        auto& m = fMap_.getShortnameMap();
        auto it = m.find(id);
        return it == m.end() ? 0.0f : *it->second;
    }

    // in/out: ponteiros por canal. count = número de amostras por canal.
    void process(int count, float** in, float** out) { fDsp_.compute(count, in, out); }

private:
    CodexDelay fDsp_;
    MapUI fMap_;
};

}  // namespace codex
