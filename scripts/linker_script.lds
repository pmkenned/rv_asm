OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS {
/*  . = 0x80000000; */
  . = 0x0;

  .text : {
    *(.text)
  }
}
