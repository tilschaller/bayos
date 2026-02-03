OUTPUT_FORMAT(binary)

ENTRY(_start)

MEMORY {
	bootloader (rwx) : ORIGIN = 0x7c00, LENGTH = 0xffff
}

SECTIONS {
	.bootloader : {
		*(.bootloader)
	} > bootloader
	/DISCARD/ : { *(*) }
}
