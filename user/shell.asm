global _start

section .text

_start:
	; do a write syscall
	mov rbx, message
	mov rcx, len
	mov rax, 4

	int 0x80

_end:
	jmp _end

section .data

message:
	db "Hello, from Userspace", 10
	len equ $ - message

