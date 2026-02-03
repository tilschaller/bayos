BITS 16
GLOBAL _start
SECTION .bootloader

_start:
	jmp 0:_bootloader
	times 8 - ($-$$) db 0

	bi_PrimaryVolumeDescriptor  resd  1    ; LBA of the Primary Volume Descriptor
	bi_BootFileLocation         resd  1    ; LBA of the Boot File
	bi_BootFileLength           resd  1    ; Length of the boot file in bytes
	bi_Checksum                 resd  1    ; 32 bit checksum
	bi_Reserved                 resb  40   ; Reserved 'for future standardization'

_bootloader:
	cli

	; zero out segment registers
	xor ax, ax
	mov gs, ax
	mov fs, ax
	mov es, ax
	mov ds, ax
	mov ss, ax

	; move the stack pointer
	mov ax, 0x7c00
	mov sp, ax
	
	; set cs to zero
	jmp 0:__after_jmp
__after_jmp:
	
	; save boot disk
	mov [_boot_disk], dl

	; print confirmation
	mov al, 'b'
	call _print_char

	; activate a20
	in al, 0x92
	test al, 2
	jnz __a20_activated
	or al, 2
	and al, 0xfe
	out 0x92, al
__a20_activated:
	mov al, 'a'
	call _print_char

	; fetch a20 memory map
	mov di, _memory_map + 4
	xor bp, bp
	mov edx, 0x534d4150
	mov eax, 0xe820
	mov [es:di + 20], dword 1
	mov ecx, 24
	int 0x15
	jc _halt_and_catch_fire
	test ebx, ebx
	je _halt_and_catch_fire
	jmp __e820_start
__e820_loop:
	mov eax, 0xe820
	mov [es:di + 20], dword 1
	mov ecx, 24
	int 0x15
	jc __e820_end
	mov edx, 0x534d4150
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
	mov [_memory_map], bp

	mov al, 'm'
	call _print_char

	; get the framebuffer
	; first get the vbe info
	mov di, _vbe_info
	mov ax, 0x4f00
	int 0x10
	cmp ax, 0x4f
	jne _halt_and_catch_fire
	xor bp, bp
	; parse the video modes
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

	pop cx

	test word [_vbe_mode_info], 1 << 7
	jz __vesa_mode_info_loop
	cmp dword [_vbe_mode_info + 0x28], 0
	je __vesa_mode_info_loop
	cmp byte [_vbe_mode_info + 0x1b], 6
	jne __vesa_mode_info_loop

	movzx eax, word [_vbe_mode_info + 18]
	movzx ebx, word [_vbe_mode_info + 20]
	mul ebx
	movzx ebx, byte [_vbe_mode_info + 24]
	mul ebx

	cmp eax, [_video_mode_score]
	jle __vesa_mode_info_loop

	mov [_video_mode_pick], cx
	mov [_video_mode_score], eax

	jmp __vesa_mode_info_loop

__enable_video_mode:
	cmp word [_video_mode_pick], 0
	je _halt_and_catch_fire

	mov cx, [_video_mode_pick]
	mov ax, 0x4f01
	mov di, _vbe_mode_info
	int 0x10
	cmp ax, 0x4f
	jne _halt_and_catch_fire

	mov bx, [_video_mode_pick]
	or bx, 0x4000
	mov ax, 0x4f02
	int 0x10
	cmp ax, 0x4f
	jne _halt_and_catch_fire

	mov al, 'v'
	call _print_char

	; parse the iso to find kernel elf
	mov eax, [bi_PrimaryVolumeDescriptor]
	call _read_disk
	push eax

	mov al, 'i'
	call _print_char

	; check if we have actually found the PrimaryVolumeDesc
	pop eax
	mov ebx, eax
	cmp byte [eax], 1
	jne _halt_and_catch_fire
	inc eax
	cmp byte [eax], 'C'
	jne _halt_and_catch_fire
	inc eax
	cmp byte [eax], 'D'
	jne _halt_and_catch_fire
	inc eax
	cmp byte [eax], '0'
	jne _halt_and_catch_fire
	inc eax
	cmp byte [eax], '0'
	jne _halt_and_catch_fire
	inc eax
	cmp byte [eax], '1'
	jne _halt_and_catch_fire

	; move address of lba of root dir lba into ebx
	; length of root dir length into ecx
	mov eax, ebx
	add eax, 158
	mov ebx, [eax]
	add eax, 8
	mov ecx, [eax]

	; can we load root dir into mem with one _read_dir call
	cmp ecx, 2048
	jg _halt_and_catch_fire
	push ecx

	mov eax, ebx
	call _read_disk
	push eax

	mov al, 'r'
	call _print_char

	; esi contains pointer to root dir
	; ecx length of root dir
	pop esi
	pop ecx

	xor edx, edx

__next_record:
	; finding the kernel is really scuffed
	; ebx contains file name pointer
	lea ebx, [esi + 33]
	cmp byte [ebx], 'K'
	jne __not_kernel
	inc ebx
	cmp byte [ebx], 'E'
	jne __not_kernel
	inc ebx
	cmp byte [ebx], 'R'
	jne __not_kernel
	inc ebx
	cmp byte [ebx], 'N'
	jne __not_kernel
	inc ebx
	cmp byte [ebx], 'E'
	jne __not_kernel
	inc ebx
	cmp byte [ebx], 'L'
	jne __not_kernel
	lea ebx, [esi + 2]
	push dword [ebx]
	jmp __found_kernel

__not_kernel:
	inc edx
	; go to next record
	movzx ebx, byte [esi]
	add esi, ebx
	jmp __next_record

__found_kernel:
	mov al, 'k'
	call _print_char
	
	; eax contains the lba of kernel
	pop eax
	push eax

	; read kernel into memory
	call _read_disk
	
	; save the kernel start
	mov edx, [eax + 24]
	mov dword [_kernel_start], edx
	mov edx, [eax + 28]
	mov dword [_kernel_start + 4], edx

	; get the address of program headers
	mov ebx, [eax + 32]
	add eax, ebx

	mov [__default_dap_size], 1

	; filesize of program header
	mov ecx, [eax + 32]
	shr ecx, 11
	inc ecx

	mov ebx, [eax + 8]
	shr ebx, 11
	pop eax
	add eax, ebx

	push ecx

	mov edx, 0x1000
	; edx contains address to which we need to read (segment)
	; ecx contains the number of sectors we need to read
	; eax conatins the lba of the ph
	mov word [__default_dap_size], 1
	mov dword [__default_dap_lba + 4], 0
	mov word [__default_dap_offset], 0
__copy_ph:
	push eax
	push ecx
	push edx

	mov word [__default_dap_segment], dx
	mov dword [__default_dap_lba], eax

	mov ah, 0x42
	mov dl, [_boot_disk]
	mov si, _default_dap

	int 0x13
	jc _halt_and_catch_fire
	
	pop edx
	pop ecx
	pop eax

	inc eax
	add edx, 0x80

	loop __copy_ph

	mov al, 'p'
	call _print_char

	; enter protected mode
	lgdt [_gdt_pro_poi]
	mov eax, cr0
	or al, 1
	mov cr0, eax

	jmp 0x8:_protected_mode

	hlt

_gdt_pro:
	dq 0
	dq 0x00CF9A000000FFFF
	dq 0x00CF92000000FFFF

_gdt_pro_poi:
	dw 23
	dq _gdt_pro

; char to print is passed from al
_print_char:
	mov ah, 0x0e
	int 0x10
	ret

_default_dap:
	db 16
	db 0
__default_dap_size:
	dw 4
__default_dap_offset:
	dw 0
__default_dap_segment:
	dw 0
__default_dap_lba:
	dq 0

; reads four sectors from the disk
; into address contained in eax upon return
; pass the lba in eax
_read_disk:
	mov word [__default_dap_offset], 0
	mov word [__default_dap_segment], 0x1000
	mov dword [__default_dap_lba], eax
	mov dword [__default_dap_lba + 4], 0
	mov ah, 0x42
	mov dl, [_boot_disk]
	mov si, _default_dap
	int 0x13
	jc _halt_and_catch_fire
	mov eax, 0x10000
	ret

_halt_and_catch_fire:
	hlt
	mov al, 'e'
	call _print_char
	hlt

_boot_disk:
	db 0

_memory_map:
	dw 0
	times 24 * 128 db 0

_vbe_info:
	db "VBE2"
	times 512 - 4 db 0
_vbe_mode_info:
	times 256 db 0

_video_mode_pick:
	dw 0
_video_mode_score:
	dd 0

BITS 32

_gdt_long:
	dq 0
	dq 0x00AF9A000000FFFF
	dq 0x00AF92000000FFFF
_gdt_long_poi:
	dw 23
	dq _gdt_long

_protected_mode:
	; reload segment registers
	mov eax, 0x10
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	mov ss, eax

	; copy program header of kernel
	; to 2mb
	pop ecx
	shl ecx, 11
	mov esi, 0x10000
	mov edi, 0x200000

	cld
	rep movsb

	; create page tables
	; i coded this in c and compiled to gnu asm 
	; and then translated to nasm
	; so not so good code quality 
	mov dword [1052664], 1052675
	mov ebx, 1056768
	xor edx, edx
	mov eax, 2097152
	mov dword [1052668], 0
	mov dword [1056752], 1056771
	mov dword [1056756], 0
	mov dword [1056760], 1060867
	mov dword [1056764], 0
.L2:
	mov esi, eax
	mov dword [ebx+4], edx
	or esi, 131
	add eax, 2097152
	mov ecx, eax
	adc edx, 0
	mov dword [ebx], esi
	add ebx, 8
	xor ecx, -2145386496
	or ecx, edx
	jne .L2
	mov dword [1048576], 1069059
	xor ecx, ecx
	xor eax, eax
	xor edx, edx
	mov esi, 1073741824
	xor edi, edi
	mov ebx, 1073152
	mov dword [1048580], 0
	mov dword [1069056], 1073155
	mov dword [1069060], 0
.L4:
	mov ebp, ebx
	mov dword [esp], ecx
	mov dword [1064964 + ecx*8], 0
	or ebp, 3
	mov dword [esp+4], ebx
	mov dword [1064960 + ecx*8], ebp
	mov ebp, ebx
align 64
align 16
align 8
.L3:
	mov ecx, eax
	mov dword [ebp+4], edx
	mov ebx, edi
	or cl, -125
	add eax, 2097152
	adc edx, 0
	mov dword [ebp], ecx
	mov ecx, esi
	add ebp, 8
	xor ecx, eax
	xor ebx, edx
	or ecx, ebx
	jne .L3
	mov ecx, dword [esp]
	mov ebx, dword [esp+4]
	add ecx, 1
	add ebx, 4096
	cmp ecx, 250
	je .L9
	add esi, 1073741824
	adc edi, 0
	jmp .L4
.L9:
	mov dword [1050624], 1064963
	mov dword [1050628], 0

	mov eax, 0x100000
	mov cr3, eax

	mov eax, cr4
	or eax, 32
	mov cr4, eax

	mov ecx, 0xc0000080
	rdmsr
	or eax, 256
	wrmsr

	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

	lgdt [_gdt_long_poi]
	jmp 0x8: _long_mode

	hlt

_kernel_stack:
	times 0x1000 db 0

_kernel_start:
	dq 0

BITS 64
_long_mode:
	mov rax, 0x10
	mov ds, rax
	mov es, rax
	mov fs, rax
	mov gs, rax
	mov ss, rax

	mov esp, _kernel_stack
	mov rax, 0xffff800000000000
	add rsp, rax
	mov rbp, rsp
	xor rdi, rdi
	xor rsi, rsi
	mov edi, _memory_map
	mov esi, _vbe_mode_info
	mov rax, [abs _kernel_start]

	jmp rax

	hlt
