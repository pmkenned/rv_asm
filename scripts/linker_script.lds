OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS {
  . = 0x0;
  . = 0x80000000;

  .text : {
    *(.text)
  }
  PROVIDE(_etext = .);

  .data : {
    *(.data)
  }

  .sdata : {
    PROVIDE(__global_pointer$ = . + 0x7C0);
    *(.sdata)
  }

  .bss : {
    *(.bss)
  }
  PROVIDE(_ebss = .);

  PROVIDE(_sp = ALIGN(_ebss + 1048576 - 7, 8));
}
