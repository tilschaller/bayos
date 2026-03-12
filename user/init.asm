global _start

section .text

_start:
	; do a write syscall
	mov rbx, message
	mov rcx, len
	mov rax, 1

	int 0x80

_loop:

	; do a read syscall
	mov rbx, message
	mov rcx, 1
	mov rax, 0

	int 0x80

	; print input
	mov rbx, message
	mov rcx, 1
	mov rax, 1

	int 0x80

	jmp _loop
	
	; exit syscall
	mov rax, 2
	int 0x80

section .data

message:
	db "Hello, from Userspace", 10
	len equ $ - message


int 0x80
