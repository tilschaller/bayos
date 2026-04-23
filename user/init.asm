global _start

section .text

_start:
	mov rax, 1
	mov rdi, 1
	mov rsi, message
	mov rdx, len
	syscall

_loop:
	mov rax, 0
	mov rdi, 0
	mov rsi, message
	mov rdx, 1
	syscall

	mov rax, 1
	mov rdi, 1
	mov rsi, message
	mov rdx, 1
	syscall

	jmp _loop

section .data

message:
	db "Hello, from Userspace", 10
	len equ $ - message
