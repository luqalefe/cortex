/* Linker do Cortex Pack para a GP-5 (DSP Cortex-M4F).
 *
 * Derivado de platform/dsp/link.ld do OpenGP5. ÚNICA mudança de memória: o .bss da
 * unidade do pack (pack.o) vai para o banco grande LARGE (122 KB) — os efeitos do
 * GuitarPedal usam ~81 KB de variáveis static comuns e a região normal VARS tem só
 * ~19,5 KB. O .bss da plataforma continua em VARS. O .data fica unificado em VARS
 * (a startup da plataforma só copia esse). O .bss do pack, por ficar dentro da seção
 * de saída .large, é zerado pela mesma rotina que zera o bloco grande.
 */
ENTRY(reset_handler)
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x4400,     LENGTH = 0x10348 - 0x4400
  VARS  (rwx) : ORIGIN = 0x20000E00, LENGTH = 0x20005C00 - 0x20000E00
  LARGE (rw)  : ORIGIN = 0x10000000, LENGTH = 0x1001E800 - 0x10000000
}
SECTIONS
{
  .text : { KEEP(*(.vectors)) *(.text*) *(.rodata*) . = ALIGN(4); } > FLASH
  .data : { __data_start = .; *(.data*) . = ALIGN(4); __data_end = .; } > VARS AT > FLASH
  __data_load = LOADADDR(.data);

  /* .bss da plataforma em VARS; .bss (e COMMON) do pack vai para LARGE. */
  .bss (NOLOAD) : {
    __bss_start = .;
    *(EXCLUDE_FILE(*pack.o) .bss*)
    *(EXCLUDE_FILE(*pack.o) COMMON)
    . = ALIGN(4); __bss_end = .;
  } > VARS

  .large (NOLOAD) : {
    __large_start = .;
    *(.large*)
    *pack.o(.bss* COMMON)
    . = ALIGN(4);
    __large_end = .;
  } > LARGE

  /DISCARD/ : { *(.ARM.exidx*) *(.comment) *(.ARM.attributes) }
}
