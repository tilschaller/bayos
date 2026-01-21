OUTPUT_FORMAT(binary)

ENTRY(_start)

MEMORY {
	boot_sector (rwx) : ORIGIN = 0x7c00, LENGTH = 446
	part_table (rwx) : ORIGIN = 0x7dbe, LENGTH = 64
	magic_bytes (rwx) : ORIGIN = 0x7dfe, LENGTH = 2
	bootloader (rwx) : ORIGIN = 0x7e00, LENGTH = 0xffff
}

SECTIONS {
	.boot_sector : {
		*(.boot_sector)
	} > boot_sector
	.part_table : {
		*(.part_table)
	} > part_table
	.magic_bytes : {
		*(.magic_bytes)
	} > magic_bytes
	.bootloader : {
		*(.bootloader16)
    		*(.bootloader162)
    		*(.bootloader162var)
    		*(.bootloader32)
    		*(.bootloader32var)
	} > bootloader
	_size_of_bootloader = SIZEOF(.bootloader);
	/DISCARD/ : { *(*) }
}
