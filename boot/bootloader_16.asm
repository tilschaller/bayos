BITS 16

GLOBAL _bootloader
EXTERN _size_of_bootloader
EXTERN bootloader_32

SECTION .bootloader16

_bootloader:
	; retrieve boot disk from stack
	pop dx
	mov [_boot_disk], dl

	; print we arrived in second stage
	mov al, '2'
	call _print_char

	; disable nmi
	in al, 0x70
	or al, 0x80
	out 0x70, al

	; enabling a20 with int 15h
	mov ax, 0x2403
	int 0x15
	jb _halt_and_catch_fire
	cmp ah, 0
	jnz _halt_and_catch_fire

	mov ax, 0x2402
	int 0x15
	jb _halt_and_catch_fire
	cmp ah, 0
	jnz _halt_and_catch_fire
	cmp al, 1
	jz __a20_activated

	mov ax, 0x2401
	int 0x15
	jb _halt_and_catch_fire
	cmp ah, 0
	jnz _halt_and_catch_fire

__a20_activated:
	; print confirmation that a20 is activated
	mov al, 'a'
	call _print_char

  ; fetch e820 memory map
  mov di, _memory_map
  xor bp, bp
  mov edx, 0x0534D4150
  mov eax, 0xe820
  mov [es:di + 20], dword 1
  mov ecx, 24 
  int 0x15
  jc _halt_and_catch_fire
  mov edx, 0x0534D4150
  cmp eax, edx
  jne _halt_and_catch_fire
  test ebx, ebx
  je _halt_and_catch_fire
  jmp __e820_start
__e820_loop:
  mov eax, 0xe820
  mov [es:di + 20], dword 1
  mov ecx, 24
  int 0x15
  jc __e820_end
  mov edx, 0x0534D4150
__e820_start:
  jcxz __e820_skip_ent
  cmp cl, 20
  jbe __e820_eval_ent
  test byte [es:di + 20], 1
  je __e820_skip_ent
__e820_eval_ent:
  mov ecx, [es:di + 8]
  or ecx, [es:di + 12]
  jz __e820_skip_ent
  inc bp
  add di, 24
__e820_skip_ent:
  test ebx, ebx
  jne __e820_loop
__e820_end:
  mov [_memory_map_entries], bp
  
  mov al, 'm'
  call _print_char

  ; we now have a memory map
  ; next get a framebuffer
  mov di, _vbe_info
  mov ax, 0x4f00
  int 0x10
  cmp ax, 0x4f
  jne _halt_and_catch_fire
  xor bp, bp
  ; parse video modes
__vesa_mode_info_loop:
  mov si, [_vbe_info + 14]
  mov ax, [_vbe_info + 16]
  mov ds, ax
  mov cx, [ds:si + bp]
  xor ax, ax
  mov ds, ax
  cmp cx, 0xffff
  je __enable_video_mode
  push cx
  add bp, 2

  mov ax, 0x4f01
  mov di, _vbe_mode_info
  int 0x10
  cmp ax, 0x4f
  jne _halt_and_catch_fire

  ; check attributes of vbe mode
  ; and pick the best
  ; first check things needed
  ; linear framebuffer support
  test word [_vbe_mode_info], 1 << 7
  jz __vesa_mode_info_loop
  ; framebuffer != 0
  cmp dword [_vbe_mode_info + 0x28], 0
  je __vesa_mode_info_loop
  ; direct/true color memory model
  cmp byte [_vbe_mode_info + 0x1b], 6
  jne __vesa_mode_info_loop

  ; score is formed by width * height * bpp
  movzx eax, word [_vbe_mode_info + 18]
  movzx ebx, word [_vbe_mode_info + 20]
  mul ebx
  movzx ebx, byte [_vbe_mode_info + 24]
  mul ebx

  ; is lower ?
  pop cx
  cmp eax, [_save_video_mode_score]
  jle __vesa_mode_info_loop

  ; replace previous winner
  mov [_save_video_mode], cx
  mov [_save_video_mode_score], eax 

__enable_video_mode:
  ; no suitable video mode was found
  cmp word [_save_video_mode], 0
  je _halt_and_catch_fire

  ; confirm a mode was found
  mov al, 'v'
  call _print_char

  ; store the mode in _vbe_video_mode
  mov cx, [_save_video_mode]
  mov ax, 0x4f01
  mov di, _vbe_mode_info
  int 0x10
  cmp ax, 0x4f
  jne _halt_and_catch_fire

  ; enable the mode
  mov bx, [_save_video_mode]
  or bx, 0x4000
  mov ax, 0x4f02
  int 0x10
  cmp ax, 0x4f
  jne _halt_and_catch_fire

  mov eax, _memory_map_entries
  mov ebx, _vbe_mode_info
  mov ecx, _size_of_bootloader
  movzx edx, byte [_boot_disk]

  jmp bootloader_32

  hlt

_boot_disk:
	db 0

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

_memory_map_entries:
  dw 0
_memory_map:
  times 24 * 128 db 0

_vbe_info:
  db "VBE2"
  times 512 - 4 db 0
_vbe_mode_info:
  times 256 db 0

_save_video_mode:
  dw 0
_save_video_mode_score:
  dd 0

_gdtr:
  dw _gdt_end - _gdt - 1
  dd _gdt

_gdt: 
  dq 0
  dq 0x00CF9A000000FFFF
  dq 0x00CF92000000FFFF
_gdt_end: