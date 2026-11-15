// Backend de áudio para Mac (e outros desktops) via miniaudio → CoreAudio.
// Implementa codex::AudioBackend. Abre um dispositivo full-duplex em f32, converte
// entre o formato entrelaçado do miniaudio e os ponteiros por canal da HAL.
#pragma once
#include <cstring>
#include <string>
#include <vector>

#include "hal.h"
#include "miniaudio.h"

namespace codex {

class MiniaudioBackend : public AudioBackend {
public:
    // inName/outName: trecho do nome do dispositivo (vazio = padrão do sistema).
    MiniaudioBackend(std::string inName, std::string outName, double wantSR, uint32_t channels)
        : inName_(std::move(inName)), outName_(std::move(outName)),
          wantSR_(wantSR), channels_(channels) {}

    ~MiniaudioBackend() override { stop(); }

    static void listDevices() {
        ma_context ctx;
        if (ma_context_init(nullptr, 0, nullptr, &ctx) != MA_SUCCESS) return;
        ma_device_info* play; ma_uint32 nPlay; ma_device_info* cap; ma_uint32 nCap;
        if (ma_context_get_devices(&ctx, &play, &nPlay, &cap, &nCap) == MA_SUCCESS) {
            std::printf("Entradas (capture):\n");
            for (ma_uint32 i = 0; i < nCap; ++i)
                std::printf("  [%u] %s%s\n", i, cap[i].name, cap[i].isDefault ? "  (padrão)" : "");
            std::printf("Saídas (playback):\n");
            for (ma_uint32 i = 0; i < nPlay; ++i)
                std::printf("  [%u] %s%s\n", i, play[i].name, play[i].isDefault ? "  (padrão)" : "");
        }
        ma_context_uninit(&ctx);
    }

    bool start(AudioCallback cb) override {
        cb_ = std::move(cb);
        if (ma_context_init(nullptr, 0, nullptr, &ctx_) != MA_SUCCESS) return false;
        ctxInit_ = true;

        ma_device_config cfg = ma_device_config_init(ma_device_type_duplex);
        cfg.sampleRate = static_cast<ma_uint32>(wantSR_);
        cfg.capture.format = ma_format_f32;
        cfg.capture.channels = channels_;
        cfg.playback.format = ma_format_f32;
        cfg.playback.channels = channels_;
        cfg.dataCallback = &MiniaudioBackend::trampoline;
        cfg.pUserData = this;
        ma_device_id capId, playId;
        if (pickDevice(ma_device_type_capture, inName_, &capId)) cfg.capture.pDeviceID = &capId;
        if (pickDevice(ma_device_type_playback, outName_, &playId)) cfg.playback.pDeviceID = &playId;

        if (ma_device_init(&ctx_, &cfg, &dev_) != MA_SUCCESS) return false;
        devInit_ = true;
        sr_ = dev_.sampleRate;
        chIn_ = dev_.capture.channels;
        chOut_ = dev_.playback.channels;
        inBuf_.assign(chIn_, {});
        outBuf_.assign(chOut_, {});
        descr_ = std::string("CoreAudio duplex, ") + std::to_string(sr_) + " Hz, in " +
                 dev_.capture.name + " / out " + dev_.playback.name;
        return ma_device_start(&dev_) == MA_SUCCESS;
    }

    void stop() override {
        if (devInit_) { ma_device_uninit(&dev_); devInit_ = false; }
        if (ctxInit_) { ma_context_uninit(&ctx_); ctxInit_ = false; }
    }

    double sampleRate() const override { return sr_; }
    std::string description() const override { return descr_; }

private:
    bool pickDevice(ma_device_type type, const std::string& name, ma_device_id* out) {
        if (name.empty()) return false;
        ma_device_info* play; ma_uint32 nPlay; ma_device_info* cap; ma_uint32 nCap;
        if (ma_context_get_devices(&ctx_, &play, &nPlay, &cap, &nCap) != MA_SUCCESS) return false;
        ma_device_info* list = (type == ma_device_type_capture) ? cap : play;
        ma_uint32 n = (type == ma_device_type_capture) ? nCap : nPlay;
        for (ma_uint32 i = 0; i < n; ++i)
            if (std::string(list[i].name).find(name) != std::string::npos) { *out = list[i].id; return true; }
        return false;
    }

    static void trampoline(ma_device* d, void* pOut, const void* pIn, ma_uint32 frames) {
        static_cast<MiniaudioBackend*>(d->pUserData)->onData(pOut, pIn, frames);
    }

    void onData(void* pOut, const void* pIn, ma_uint32 frames) {
        const float* in = static_cast<const float*>(pIn);
        float* out = static_cast<float*>(pOut);
        for (auto& v : inBuf_) { v.resize(frames); }
        for (auto& v : outBuf_) { v.resize(frames); std::fill(v.begin(), v.end(), 0.0f); }
        for (ma_uint32 i = 0; i < frames; ++i)
            for (ma_uint32 c = 0; c < chIn_; ++c) inBuf_[c][i] = in[i * chIn_ + c];

        std::vector<const float*> inp(chIn_);
        std::vector<float*> outp(chOut_);
        for (ma_uint32 c = 0; c < chIn_; ++c) inp[c] = inBuf_[c].data();
        for (ma_uint32 c = 0; c < chOut_; ++c) outp[c] = outBuf_[c].data();
        cb_(inp.data(), outp.data(), frames, chOut_);

        for (ma_uint32 i = 0; i < frames; ++i)
            for (ma_uint32 c = 0; c < chOut_; ++c) out[i * chOut_ + c] = outBuf_[c][i];
    }

    std::string inName_, outName_, descr_;
    double wantSR_;
    uint32_t channels_;
    AudioCallback cb_;
    ma_context ctx_{};
    ma_device dev_{};
    bool ctxInit_ = false, devInit_ = false;
    double sr_ = 0;
    ma_uint32 chIn_ = 0, chOut_ = 0;
    std::vector<std::vector<float>> inBuf_, outBuf_;
};

}  // namespace codex
