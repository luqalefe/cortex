/* Codex Delay — metade DSP, alvo GP-5 (backend: platform do OpenGP5).
 *
 * Isto é um firmware DSP do OpenGP5 (contrato: gp5_dsp.h). Ele NÃO contém o efeito:
 * só faz a ponte entre o mundo do OpenGP5 (áudio int32, settings numerados) e o
 * efeito portátil do Codex (effects/delay/codex_delay.h, áudio float).
 *
 * O mesmo codex_delay.h roda no Mac pelo motor do Codex. Aqui ele é compilado no
 * perfil da GP-5 (memória de delay em int16, teto de ~1 s).
 *
 * Settings vindos da metade de controle (ver ../nds32/firmware.c), inteiros:
 *   id 0  time    ms            (1 .. 1000)
 *   id 1  repeats x1000         (0 .. 950)
 *   id 2  mix     x1000         (0 .. 1000)
 *   id 3  tone    Hz            (800 .. 16000)
 */
#define CODEX_TARGET_GP5
#define CODEX_MAX_DELAY_MS 1000           /* cabe nos 122 KB em int16 */
#include "codex_delay.h"                  /* caminho dado por -I no build; ver README */

static void fw_init(void) { codex_delay_init((float)GP5_SAMPLE_RATE); }

static void fw_process(const int32_t *in, int32_t *out_left, int32_t *out_right) {
    float fin[GP5_BLOCK], fL[GP5_BLOCK], fR[GP5_BLOCK];
    for (uint32_t i = 0; i < GP5_BLOCK; i++) fin[i] = (float)in[i] / GP5_FULL_SCALE;
    codex_delay_process(fin, fL, fR, (int)GP5_BLOCK);
    for (uint32_t i = 0; i < GP5_BLOCK; i++) {
        float l = fL[i], r = fR[i];
        if (l >  1.0f) l =  1.0f; if (l < -1.0f) l = -1.0f;
        if (r >  1.0f) r =  1.0f; if (r < -1.0f) r = -1.0f;
        out_left[i]  = (int32_t)(l * GP5_FULL_SCALE * 0.999f);
        out_right[i] = (int32_t)(r * GP5_FULL_SCALE * 0.999f);
    }
}

static int fw_command(uint32_t cmd, const uint8_t *p, uint32_t n) {
    if (cmd != 0x7F10 || n < 12) return 0;          /* não é setting: o platform responde */
    uint32_t id = gp5_get32(p + 4), value = gp5_get32(p + 8);
    switch (id) {
        case 0: codex_delay_set_index(0, (float)value);            break;  /* time ms */
        case 1: codex_delay_set_index(1, (float)value / 1000.0f);  break;  /* repeats */
        case 2: codex_delay_set_index(2, (float)value / 1000.0f);  break;  /* mix */
        case 3: codex_delay_set_index(3, (float)value);            break;  /* tone Hz */
        default: return 0;
    }
    gp5_ack();
    return 1;
}
