/* Cortex Pack — firmware DSP da GP-5 (contrato gp5_dsp.h do OpenGP5).
 *
 * Fino de propósito: só liga o contrato do OpenGP5 (fw_init/fw_process/fw_command)
 * à API do pack (pack.c, TU separada), que roda o core de áudio do GuitarPedal.
 * Compilado dentro do responder.c do OpenGP5.
 */
#include <stdint.h>

/* API implementada em pack.c */
void pack_init(void);
void pack_process(const int32_t *in, int32_t *outL, int32_t *outR, int n);
void pack_set_pot(int effect_id, int pot_idx, int val);

static void fw_init(void) { pack_init(); }

static void fw_process(const int32_t *in, int32_t *out_left, int32_t *out_right) {
    pack_process(in, out_left, out_right, (int)GP5_BLOCK);
}

static int fw_command(uint32_t cmd, const uint8_t *p, uint32_t n) {
    if (cmd != 0x7F10 || n < 12) return 0;
    uint32_t id = gp5_get32(p + 4), val = gp5_get32(p + 8);
    /* id: byte alto = índice do efeito, byte baixo = índice do pot */
    pack_set_pot((int)(id >> 8), (int)(id & 0xFF), (int)val);
    gp5_ack();
    return 1;
}
