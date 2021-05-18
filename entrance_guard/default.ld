OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)
SECTIONS
{
  _stext = .;
  .text :
  {
    KEEP (*(SORT_NONE(.Reset_Handler)))
    *(.text.*)
  }
  _etext = .;
  .rodata         : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
  .rodata1        : { *(.rodata1) }
  .got            : { *(.got.plt) *(.igot.plt) *(.got) *(.igot) }
  .data           :
  {
    __data_start = . ;
    *(.data .data.* .gnu.linkonce.d.*)
  }
  __bss_start = .;
  .bss :
  {
   *(.dynbss)
   *(.bss .bss.* .gnu.linkonce.b.*)
   *(COMMON)
  }
  _bss_end__ = .;
  _sstack = .;
  .stack :
  {
    _stack = .;
    *(.stack)
  }
  _estack = .;
  /DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) }
}
