/* Codex Delay — metade de controle, alvo GP-5 (backend: platform do OpenGP5).
 *
 * Mapeia os controles físicos da GP-5 nos settings do efeito (ver ../dsp/firmware.h):
 *   knob de volume  -> mix          (id 2)
 *   PARA girando     -> time         (id 0), 20 ms por clique
 *   footswitch       -> bypass: liga/desliga o molhado (mix vai a 0 quando desligado)
 * repeats e tone ficam nos padrões, enviados uma vez quando o DSP acorda.
 *
 * Contrato: platform/nds32/gp5_firmware.h. Nenhuma função pode bloquear.
 */
#include "gp5_firmware.h"

enum { ID_TIME = 0, ID_REPEATS = 1, ID_MIX = 2, ID_TONE = 3, NSET = 4 };

static uint32_t val[NSET];      /* valor atual de cada setting (inteiros, ver dsp/firmware.h) */
static uint32_t changed;        /* bitmask de settings a enviar */
static unsigned knob;           /* última posição do knob (0..255) */
static int      on = 1;         /* bypass: 1 = delay ativo */
static unsigned shown_time = 0xFFFF;

static void mark(int id, uint32_t v) { val[id] = v; changed |= (1u << id); }

void fw_init(void) {
    val[ID_TIME] = 480; val[ID_REPEATS] = 400; val[ID_MIX] = 350; val[ID_TONE] = 6000;
    changed = (1u << NSET) - 1;   /* envia todos */
    knob = 0; on = 1; shown_time = 0xFFFF;
}

void fw_knob(unsigned k) {
    knob = k;
    if (on) mark(ID_MIX, (k * 1000u) / 255u);   /* 0..255 -> 0..1000 */
}

void fw_para_turn(int detents) {
    long t = (long)val[ID_TIME] + (long)detents * 20;   /* 20 ms por clique */
    if (t < 1)    t = 1;
    if (t > 1000) t = 1000;
    mark(ID_TIME, (uint32_t)t);
}

void fw_para_push(void) {}

void fw_foot(void) {
    on = !on;
    mark(ID_MIX, on ? (knob * 1000u) / 255u : 0u);   /* bypass simples: corta o molhado */
}

unsigned fw_level(void) { return 255; }   /* volume de saída cheio; o mix é o knob */

void fw_dsp_ready(void) { changed = (1u << NSET) - 1; }   /* DSP reiniciou: reenvia tudo */

int fw_take(uint32_t *id, uint32_t *value) {
    for (int i = 0; i < NSET; i++)
        if (changed & (1u << i)) {
            changed &= ~(1u << i);
            *id = (uint32_t)i; *value = val[i];
            return 1;
        }
    return 0;
}

unsigned fw_setting_count(void) { return NSET; }

unsigned fw_active(void) { return on ? 1u : 0u; }   /* cor do LED: ativo/bypass */

int fw_draw(unsigned k, int force) {
    (void)k;
    char line[14] = "TIME     0000";
    if (force) {
        gp5_text(0, on ? "CODEX DELAY  " : "  BYPASS     ");
        gp5_text(1, "             ");
        gp5_text(3, "VOL = MIX    ");
        gp5_text(4, "PARA = TIME  ");
        shown_time = 0xFFFF;
    }
    if (val[ID_TIME] != shown_time) {
        unsigned t = val[ID_TIME];
        line[9]  = '0' + (t / 1000) % 10;
        line[10] = '0' + (t / 100)  % 10;
        line[11] = '0' + (t / 10)   % 10;
        line[12] = '0' + t % 10;
        gp5_text(2, line);
        shown_time = t;
    }
    return 0;
}
