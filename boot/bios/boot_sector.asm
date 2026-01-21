BITS 16
GLOBAL _start
SECTION .boot_sector

EXTERN _size_of_bootloader

_start:
	; disable interrupts
	cli

	; zero out the segment registers
	xor ax, ax
	mov gs, ax
	mov fs, ax
	mov es, ax
	mov ds, ax
	mov ss, ax

	; move the stack pointer to 0x7c00
	mov ax, 0x7c00
	mov sp, ax

	; set cs to zero
	jmp 0:__after_jmp
__after_jmp:

	; save boot disk
	mov [_boot_disk], dl

	mov al, 'n'
	call _print_char

	; calculate the number of segment we need to load for stage 2
	; align the address to 512
	mov eax, _size_of_bootloader
	add eax, 511
	and eax, ~511
	; divide eax by 512
	mov ebx, 512
	xor edx, edx
	div ebx
	; number of segments is now stored in eax
	mov [__num_segments], ax
	; load the sectors into memory
	; TODO: maybe use a higher segment, so we can load higher addresses
	mov si, _dap_second_stage
	mov ah, 0x42
	mov dl, [_boot_disk]
	int 0x13
	; check if read was successfull
	jc _halt_and_catch_fire
	; output confirmation of success
	mov al, 'd'
	call _print_char

	; jump to second stage
	mov dl, [_boot_disk]
	push dx
	jmp 0:0x7e00

; character to print is passed through al
; registers are not preserved
_print_char:
	mov ah, 0x0e
	xor bx, bx
	int 0x10
	ret

_halt_and_catch_fire:
	mov al, 'e'
	call _print_char
	cli
	hlt

_boot_disk: 
	db 0

ALIGN 16
_dap_second_stage:
	db 16 		; size of dap
	db 0       	; reserved zero
__num_segments:
	dw 0   		; number of segment to read
	dw 0x0      ; transfer buffer offset
	dw 0x7e0 	; transfer buffer segment
	dd 1 		; lower lba of disk
	dd 0 		; higher lba of disk

SECTION .part_table
_part_table:
	db 0x80
	db 0x00, 0x02, 0x00
	db 0x0c
	db 0x00, 0x20, 0x3f
	dd 0x1
	dd 0xffffff
	times 16 * 3 db 0

SECTION .magic_bytes
_magic_bytes:
	db 0x55
	db 0xaa
