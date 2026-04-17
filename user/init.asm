global _start

section .text

_start:
	mov rbx, message
	mov rcx, len
	mov rax, 1
	syscall

_loop:
	mov rbx, message
	mov rcx, 1
	mov rax, 0
	syscall

	mov rbx, message
	mov rcx, 1
	mov rax, 1
	syscall

	jmp _loop

section .data

message:
	db "Hello, from Userspace", 10
	len equ $ - message
