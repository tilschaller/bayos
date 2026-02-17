global _start

section .text

_start:
	; do a write syscall
	mov rbx, message
	mov rcx, len
	mov rax, 1

	int 0x80
	
	; exit syscall
	mov rax, 2
	int 0x80

section .data

message:
	db "Hello, from Userspace", 10
	len equ $ - message

