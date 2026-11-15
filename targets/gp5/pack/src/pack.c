/* Cortex Pack — driver do core de áudio do GuitarPedal na GP-5.
 *
 * TU separada (não colide com o responder.c do OpenGP5). Inclui o core deles,
 * emula o anel i2s/DMA que o bench emula, e expõe uma API mínima em C que o
 * firmware (firmware.h, no TU do responder) chama.
 *
 * GPLv2 — usa código do GuitarPedal (Torvalds), GPLv2-only. Ver NOTICE.md.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* protótipos (o core usa memcpy/memset; definimos abaixo, -nostdlib) */
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
void *memmove(void *, const void *, size_t);

/* shim do M4F (espelha Validation/bench/shim) */
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/timer.h"

/* registradores "DMA"/timer que o core deles lê (emulação, igual ao bench) */
struct bench_dma   bench_dma_regs;
struct bench_timer bench_timer_regs;

#include "status.h"
#include "Audio/types.h"
#include "Audio/util.h"
#include "Audio/envelope.h"
#include "Audio/single-pole.h"
#include "Audio/biquad.h"
#include "Audio/fft.h"
#include "Audio/analyze.h"
#include "Audio/effect.h"        /* traz effect_map.h: EFFECT_COUNT, effects[], acessores */

/* símbolos que o includer deve fornecer (ver o cabeçalho de Audio/effect.h) */
uint8_t effect_chain[MAX_ROUTED_EFFECTS];
uint8_t routed_effect_count = 0;

#include "effect-state.h"

/* memcpy/memset/memmove e log2f vêm da plataforma (responder.c/gp5_math.c do OpenGP5).
 * Só o que falta (não inlinado, -nostdlib): */
float fabsf(float x) { return __builtin_fabsf(x); }
float sqrtf(float x) { return __builtin_sqrtf(x); }
float floorf(float x) { float t = (float)(long)x; return (t > x) ? t - 1.0f : t; }
float rintf(float x) {
    float t = (x < 0.0f) ? -x : x;
    float r = (float)(long)(t + 0.5f);
    return (x < 0.0f) ? -r : r;
}

sample_t get_usb_audio_input(void)   { sample_t z = { 0.0f, 0.0f }; return z; }
sample_t get_radio_audio_input(void) { sample_t z = { 0.0f, 0.0f }; return z; }
void put_radio_audio_output(raw_sample_t wet, raw_sample_t dry) { (void)wet; (void)dry; }

/* ---------- utilitários sem libc ---------- */
static int cmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}
static int find_effect_id(const char *name) {
    for (int i = 0; i < EFFECT_COUNT; i++)
        if (effects[i]->name && cmp(effects[i]->name, name) == 0) return i;
    return -1;
}

/* ---------- API para o firmware ---------- */
void pack_init(void) {
    for (int i = 0; i < EFFECT_COUNT; i++) reset_effect(effects[i]);

    routing_bitmap_t r = routing_start();
    int id = find_effect_id("Boost");          /* cadeia padrão v1: um efeito leve */
    if (id >= 0) routing_add(&r, (uint8_t)id);
    routing_end(r);

    for (int i = 0; i < EFFECT_COUNT; i++) {
        struct effect *e = effects[i];
        e->last = e->seq;
        if (e->prepare) e->prepare(effect_pots(e));
        if (e->init)    e->init(effect_pots(e));
    }
    init_meters();
}

/* ajusta um pot (o core re-inicializa o efeito na borda de bloco) */
void pack_set_pot(int effect_id, int pot_idx, int val) {
    if (effect_id < 0 || effect_id >= EFFECT_COUNT) return;
    if (val < 0) val = 0;
    if (val > 255) val = 255;
    set_effect_pot(effects[effect_id], (unsigned)pot_idx, (unsigned char)val);
}

/* processa um bloco: mono int32 in -> estéreo int32 out, via o core deles */
void pack_process(const int32_t *in, int32_t *outL, int32_t *outR, int n) {
    for (int i = 0; i < n; i++) {
        unsigned slot = cpu_idx;
        i2s_dma_buf[slot].left  = in[i];
        i2s_dma_buf[slot].right = in[i];
        bench_dma_regs.ch[dma_rx].write_addr = (uintptr_t)&i2s_dma_buf[(slot + 1) & 15];
        bench_dma_regs.ch[dma_tx].read_addr  = (uintptr_t)&i2s_dma_buf[(slot + 8) & 15];
        bench_timer_regs.timerawl += 21;
        single_sample(1.0f);
        outL[i] = i2s_dma_buf[slot].left;
        outR[i] = i2s_dma_buf[slot].right;
    }
}
