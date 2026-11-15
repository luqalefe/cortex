/* Codex Delay — DSP portátil (um efeito, vários alvos).
 *
 * Delay com realimentação e amortecimento de tom, escrito em C simples:
 *   time    tempo do eco, em ms        (1 .. CODEX_MAX_DELAY_MS)
 *   repeats realimentação              (0 .. 0.95)
 *   mix     seco/molhado               (0 .. 1)
 *   tone    corte do filtro no eco, Hz (800 .. 16000)
 *
 * Instância única por tradução (estado em variáveis static), como os firmwares do OpenGP5.
 * O host (Mac) e a GP-5 incluem este header e chamam as três funções abaixo.
 * A memória do delay usa codex_store_t (float no host, int16 na GP-5) e o tamanho
 * máximo vem de CODEX_MAX_DELAY_MS — a diferença de alvo fica explícita, não escondida.
 */
#ifndef CODEX_DELAY_H
#define CODEX_DELAY_H
#include "codex_effect.h"   /* resolvido por -Ieffects (host) ou pelo staging (GP-5) */

#define CODEX_DELAY_SAMPLES ((CODEX_MAX_DELAY_MS) * 441 / 10)   /* ms * 44,1 */

static codex_store_t cdx_line[CODEX_DELAY_SAMPLES] CODEX_LARGE;
static float cdx_sr        = CODEX_SAMPLE_RATE_DEFAULT;
static int   cdx_wr        = 0;
static float cdx_time_ms   = 480.0f;   /* alvo do tempo */
static float cdx_delay_smp = 0.0f;     /* tempo suavizado, em amostras */
static float cdx_repeats   = 0.4f;
static float cdx_mix       = 0.35f;
static float cdx_tone_hz   = 6000.0f;
static float cdx_lp_c      = 1.0f;     /* coef. do filtro de 1 polo (0..1) */
static float cdx_lp_z      = 0.0f;     /* estado do filtro */

/* Tabela de parâmetros (o host usa os nomes; a GP-5 usa o índice). */
static const CodexParam codex_delay_params[] = {
    { "time",    "Time",    1.0f, (float)CODEX_MAX_DELAY_MS, 480.0f  },
    { "repeats", "Repeats", 0.0f, 0.95f,                     0.4f    },
    { "mix",     "Mix",     0.0f, 1.0f,                      0.35f   },
    { "tone",    "Tone",    800.0f, 16000.0f,                6000.0f },
};
#define CODEX_DELAY_NPARAMS 4

static void cdx_set_tone(float hz) {
    if (hz < 800.0f)   hz = 800.0f;
    if (hz > 16000.0f) hz = 16000.0f;
    cdx_tone_hz = hz;
    /* 1 polo passa-baixa: c = 1 - exp(-2*pi*fc/fs). c->1 = claro, c pequeno = escuro. */
    cdx_lp_c = 1.0f - codex_expf(-2.0f * 3.14159265f * hz / cdx_sr);
}

static void codex_delay_init(float sr) {
    if (sr > 0.0f) cdx_sr = sr;
    cdx_wr = 0; cdx_lp_z = 0.0f;
    for (int i = 0; i < CODEX_DELAY_SAMPLES; i++) cdx_line[i] = 0;
    cdx_delay_smp = cdx_time_ms * cdx_sr / 1000.0f;
    cdx_set_tone(cdx_tone_hz);
}

static float cdx_read(float samples_back) {
    float rp = (float)cdx_wr - samples_back;
    while (rp < 0.0f) rp += (float)CODEX_DELAY_SAMPLES;
    int i0 = (int)rp; float f = rp - (float)i0;
    int i1 = i0 + 1; if (i1 >= CODEX_DELAY_SAMPLES) i1 = 0;
    float a = (float)cdx_line[i0] / CODEX_STORE_MAX;
    float b = (float)cdx_line[i1] / CODEX_STORE_MAX;
    return a + (b - a) * f;   /* interpolação linear */
}

static void codex_delay_process(const float *in, float *out_left, float *out_right, int n) {
    const float maxd = (float)(CODEX_DELAY_SAMPLES - 2);
    for (int i = 0; i < n; i++) {
        float target = cdx_time_ms * cdx_sr / 1000.0f;
        if (target > maxd) target = maxd;
        if (target < 1.0f) target = 1.0f;
        cdx_delay_smp += 0.0005f * (target - cdx_delay_smp);   /* desliza p/ não clicar/pular tom */

        float dry  = in[i];
        float echo = cdx_read(cdx_delay_smp);
        cdx_lp_z  += cdx_lp_c * (echo - cdx_lp_z);              /* tom: filtra o eco */
        float wet  = cdx_lp_z;

        float fb = dry + cdx_repeats * wet;                     /* realimentação */
        if (fb >  1.0f) fb =  1.0f;
        if (fb < -1.0f) fb = -1.0f;
        cdx_line[cdx_wr] = (codex_store_t)(fb * CODEX_STORE_MAX);
        if (++cdx_wr >= CODEX_DELAY_SAMPLES) cdx_wr = 0;

        float y = dry * (1.0f - cdx_mix) + wet * cdx_mix;
        out_left[i] = y; out_right[i] = y;
    }
}

/* Ajuste por índice (sem libc — usado pela GP-5). */
static int codex_delay_set_index(int i, float v) {
    switch (i) {
        case 0: cdx_time_ms = v; return 1;
        case 1: cdx_repeats = (v < 0.0f ? 0.0f : (v > 0.95f ? 0.95f : v)); return 1;
        case 2: cdx_mix     = (v < 0.0f ? 0.0f : (v > 1.0f  ? 1.0f  : v)); return 1;
        case 3: cdx_set_tone(v); return 1;
        default: return 0;
    }
}

#ifndef CODEX_TARGET_GP5
/* Ajuste por nome (host/Mac; usa strcmp da libc). */
#include <string.h>
static int codex_delay_set(const char *id, float v) {
    for (int i = 0; i < CODEX_DELAY_NPARAMS; i++)
        if (strcmp(id, codex_delay_params[i].id) == 0)
            return codex_delay_set_index(i, v);
    return 0;
}
#endif

#endif /* CODEX_DELAY_H */
