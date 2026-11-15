// Codex Engine (alvo Mac): carrega o DSP do Faust, abre áudio full-duplex pelo
// CoreAudio e aceita controle do simulador por WebSocket.
//
// Uso:
//   codex-engine --list                 lista dispositivos de áudio
//   codex-engine --selftest             testa o DSP sem hardware (impulso) e sai
//   codex-engine [--in NOME] [--out NOME] [--sr 44100] [--port 7070] [--ch 2]
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#include "audio_miniaudio.h"
#include "engine.h"
#include "ws_server.h"

using namespace codex;

static std::atomic<bool> gRun{true};
static void onSig(int) { gRun = false; }

// --- extrator JSON mínimo (só o formato que nós mesmos definimos) ---
static bool jsonStr(const std::string& j, const char* key, std::string& out) {
    std::string pat = std::string("\"") + key + "\"";
    auto k = j.find(pat); if (k == std::string::npos) return false;
    auto c = j.find(':', k + pat.size()); if (c == std::string::npos) return false;
    auto q1 = j.find('"', c); if (q1 == std::string::npos) return false;
    auto q2 = j.find('"', q1 + 1); if (q2 == std::string::npos) return false;
    out = j.substr(q1 + 1, q2 - q1 - 1); return true;
}
static bool jsonNum(const std::string& j, const char* key, double& out) {
    std::string pat = std::string("\"") + key + "\"";
    auto k = j.find(pat); if (k == std::string::npos) return false;
    auto c = j.find(':', k + pat.size()); if (c == std::string::npos) return false;
    out = std::strtod(j.c_str() + c + 1, nullptr); return true;
}
static bool jsonBool(const std::string& j, const char* key, bool& out) {
    std::string pat = std::string("\"") + key + "\"";
    auto k = j.find(pat); if (k == std::string::npos) return false;
    auto c = j.find(':', k + pat.size()); if (c == std::string::npos) return false;
    while (j[c] == ':' || j[c] == ' ') ++c;
    out = j.compare(c, 4, "true") == 0; return true;
}

static int selftest() {
    Engine eng; eng.init(44100.0);
    const uint32_t N = 44100;  // 1 s
    std::vector<float> in(N, 0.0f), outL(N, 0.0f), outR(N, 0.0f);
    in[0] = 1.0f;  // impulso
    eng.setParam("mix", 1.0f); eng.setParam("repeats", 0.5f); eng.setParam("time", 300.0f);
    const uint32_t block = 256;
    for (uint32_t i = 0; i < N; i += block) {
        uint32_t n = std::min(block, N - i);
        const float* pin[2] = {in.data()+i, in.data()+i};
        float* pout[2] = {outL.data()+i, outR.data()+i};
        eng.process(pin, pout, n, 2);
    }
    // acha os picos do delay (ecos) na saída
    int echoes = 0; float thr = 0.05f; uint32_t last = 0;
    for (uint32_t i = 1; i < N-1; ++i)
        if (outL[i] > thr && outL[i] >= outL[i-1] && outL[i] > outL[i+1] && i - last > 100) {
            double ms = 1000.0 * i / 44100.0;
            std::printf("  eco %d em %.1f ms (amp %.3f)\n", ++echoes, ms, outL[i]);
            last = i; if (echoes >= 4) break;
        }
    bool ok = echoes >= 2;
    std::printf("DSP selftest: %s (%d ecos detectados; esperado ~a cada 300 ms)\n",
                ok ? "OK" : "FALHOU", echoes);
    return ok ? 0 : 1;
}

int main(int argc, char** argv) {
    std::string inName, outName; double sr = 44100; int port = 7070; uint32_t ch = 2;
    bool noAudio = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--list") { MiniaudioBackend::listDevices(); return 0; }
        else if (a == "--selftest") return selftest();
        else if (a == "--no-audio") noAudio = true;
        else if (a == "--in" && i+1 < argc) inName = argv[++i];
        else if (a == "--out" && i+1 < argc) outName = argv[++i];
        else if (a == "--sr" && i+1 < argc) sr = std::atof(argv[++i]);
        else if (a == "--port" && i+1 < argc) port = std::atoi(argv[++i]);
        else if (a == "--ch" && i+1 < argc) ch = std::atoi(argv[++i]);
        else if (a == "--help") { std::printf("ver cabeçalho de main.cpp\n"); return 0; }
    }

    Engine eng; eng.init(sr);
    MiniaudioBackend audio(inName, outName, sr, ch);
    WsServer ctl(static_cast<uint16_t>(port));

    ctl.start([&eng](const std::string& msg) {
        std::string id; double v; bool b;
        if (jsonStr(msg, "param", id) && jsonNum(msg, "value", v)) eng.setParam(id, (float)v);
        if (jsonBool(msg, "bypass", b)) eng.setBypass(b);
        if (jsonBool(msg, "toggle", b) && b) eng.toggleBypass();
    });

    if (!noAudio && !audio.start([&eng](const float** in, float** out, uint32_t f, uint32_t c) {
            eng.process(in, out, f, c);
        })) {
        std::fprintf(stderr, "ERRO: não consegui abrir o áudio. Tente --list e passe --in/--out.\n");
        return 1;
    }

    std::signal(SIGINT, onSig); std::signal(SIGTERM, onSig);
    std::printf("Codex Engine rodando.\n  %s\n  controle: ws://localhost:%d\n  Ctrl+C para sair.\n",
                noAudio ? "SEM ÁUDIO (só controle)" : audio.description().c_str(), port);
    // empurra o estado ~20x/s para o simulador (medidor, bypass, params)
    while (gRun) {
        ctl.push(eng.snapshotJson());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::printf("\nencerrando...\n");
    if (!noAudio) audio.stop();
    ctl.stop();
    return 0;
}
