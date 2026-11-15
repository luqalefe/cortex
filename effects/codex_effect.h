/* Codex — contrato portátil de efeito.
 *
 * Um efeito do Codex é escrito uma vez, em C simples, e roda em vários alvos:
 *   - host/Mac  : motor do Codex (CoreAudio) e tools/run_effect do OpenGP5
 *   - GP-5      : platform do OpenGP5 (DSP Cortex-M4F), via targets/gp5
 *
 * Este header isola o efeito das diferenças de plataforma:
 *   - de onde vêm as funções de matemática (o DSP da GP-5 não tem libc)
 *   - onde ficam os buffers grandes (delay lines) e com que precisão
 *   - os limites honestos de cada alvo (ex.: memória de delay)
 *
 * Perfis:
 *   defina CODEX_TARGET_GP5  -> perfil da GP-5 (compilado dentro do firmware DSP do OpenGP5)
 *   nada definido            -> perfil portátil (host/Mac), precisão total
 */
#ifndef CODEX_EFFECT_H
#define CODEX_EFFECT_H
#include <stdint.h>

#if defined(CODEX_TARGET_GP5)
  /* Dentro do firmware DSP do OpenGP5: gp5_dsp.h já foi incluído pelo platform.
   * A matemática vem do platform (sinf/expf/...); não há <math.h>. */
  #include "gp5_math.h"
  #define CODEX_LARGE      GP5_LARGE          /* põe o buffer grande no bloco de 122 KB */
  typedef int16_t codex_store_t;              /* memória de delay em 16 bits p/ caber */
  #define CODEX_STORE_MAX  32767.0f
  #ifndef CODEX_MAX_DELAY_MS
  #define CODEX_MAX_DELAY_MS 1000             /* ~1 s mono @44,1k em int16 (~86 KB) */
  #endif
#else
  /* Perfil portátil (host/Mac): precisão total, memória à vontade. */
  #include <math.h>
  #define CODEX_LARGE                         /* nada: buffer normal */
  typedef float   codex_store_t;              /* memória de delay em float */
  #define CODEX_STORE_MAX  1.0f
  #ifndef CODEX_MAX_DELAY_MS
  #define CODEX_MAX_DELAY_MS 2000             /* 2 s no host */
  #endif
#endif

/* Shims de matemática: o DSP do efeito nunca chama libm direto. */
#ifndef codex_expf
#define codex_expf expf
#endif
#ifndef codex_sinf
#define codex_sinf sinf
#endif

#define CODEX_SAMPLE_RATE_DEFAULT 44100.0f

/* Metadado de um parâmetro de efeito (igual em todos os alvos). */
typedef struct {
    const char *id;     /* nome curto, ex.: "time" (usado pelo host) */
    const char *name;   /* rótulo, ex.: "Time" */
    float min, max, def;
} CodexParam;

#endif /* CODEX_EFFECT_H */
