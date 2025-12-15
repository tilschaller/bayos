OUTPUT_FORMAT(elf64-x86-64)

ENTRY(_start)

MEMORY {
  # higher half kernel in last 2GiB of address space
  kernel (rwx) : ORIGIN = 0xFFFFFFFF80000000, LENGTH = 0x80000000
}

SECTIONS {
  .text : {
    *(.text)
  } > kernel
  
  .rodata : {
    *(.rodata)
  } > kernel

  .data : {
    *(.data)
  } > kernel

  .bss : {
    *(.bss)
  } > kernel
}
