OUTPUT_FORMAT(binary)

ENTRY(_start)

MEMORY {
	boot_sector (rwx) : ORIGIN = 0x7c00, LENGTH = 510
	magic_bytes (rwx) : ORIGIN = 0x7dfe, LENGTH = 2
	bootloader (rwx) : ORIGIN = 0x7e00, LENGTH = 1M
}

SECTIONS {
	.bootloader : {
		*(.bootloader16)
    *(.bootloader32)
	} > bootloader
	.boot_sector : {
		*(.boot_sector)
		_size_of_bootloader = SIZEOF(.bootloader) - 0x7c00;
	} > boot_sector
	.magic_bytes : {
		*(.magic_bytes)
	} > magic_bytes
}
