%macro isr_err_stub 1
isr_stub_%+%1:
	mov rdi, %1
	pop rsi
	call exception_handler
	iretq 
%endmacro

%macro isr_no_err_stub 1
isr_stub_%+%1:
	mov rdi, %1
	xor rsi, rsi
	call exception_handler
	iretq
%endmacro

%macro pushaq 0
	push rax
	push rcx
	push rdx
	push rbx
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro popaq 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rbp
	pop rbx
	pop rdx
	pop rcx
	pop rax
%endmacro

extern exception_handler
isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31

global isr_stub_table
isr_stub_table:
%assign i 0 
%rep    32 
	dq isr_stub_%+i
%assign i i+1 
%endrep

extern timer_handler
extern spurious_handler
extern apic_error_handler
extern keyboard_handler
extern syscall_handler

global _after_timer_handler

global timer_handler_stub
global spurious_handler_stub
global apic_error_handler_stub
global keyboard_handler_stub
global syscall_handler_stub

timer_handler_stub:
	pushaq
	mov rdi, rsp
	call timer_handler
_after_timer_handler:
	mov rsp, rax
	popaq
	iretq
spurious_handler_stub:
	pushaq
	mov rdi, rsp
	call spurious_handler
	popaq
	iretq
apic_error_handler_stub:
	pushaq
	mov rdi, rsp
	call apic_error_handler_stub
	popaq
	iretq
keyboard_handler_stub:
	pushaq
	mov rdi, rsp
	call keyboard_handler
	popaq
	iretq
syscall_handler_stub:
	; input: 3 registers
	;	rax: contains syscall number
	;	rbx: first argument
	;	rcx: second argument
	; store all registers
	; TODO: look into only preserving the registers
	; restored via system v abi and discard scratch registers
	pushaq
	; arguments for syscall_handler
	; 4 uint64_t
	;	1: pointer to stack containing iretq frame
	;	2: first argument from syscall
	;	3: second argument from syscall
	;	4: third argument from syscall
	mov rdi, rsp
	mov rsi, rax
	mov rdx, rbx
	; mov rcx, rcx rcx is already right
	call syscall_handler
	popaq
	iretq
