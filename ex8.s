	.comm printf, 4, 4
	.comm a, 40, 4
	.comm main, 4, 4
	.section .rodata
.string_ro_0:
	.string "a[%d] is %d\n"
	.data
	.align 4
.string_0:
	.long .string_ro_0
	.section .rodata
.string_ro_1:
	.string "Hak is god\n"
	.data
	.align 4
.string_1:
	.long .string_ro_1
	.section .rodata
.string_ro_2:
	.string "Hak is not god\n"
	.data
	.align 4
.string_2:
	.long .string_ro_2
	.section .rodata
.string_ro_3:
	.string "Hak is not god\n"
	.data
	.align 4
.string_3:
	.long .string_ro_3
	.section .rodata
.string_ro_4:
	.string "Hak is still god\n"
	.data
	.align 4
.string_4:
	.long .string_ro_4
	.text
main:
	pushl %ebp
	movl %esp, %ebp
	subl $64, %esp
.BB.1.1:
	lea a, %ecx
	movl %ecx, -8(%ebp)
	movl $5, %eax
	xor %edx, %edx
	movl $4, %ebx
	mul %ebx
	movl %eax, -12(%ebp)
	movl -8(%ebp), %ecx
	addl -12(%ebp), %ecx
	movl %ecx, -16(%ebp)
	movl -16(%ebp), %ecx
	movl (%ecx), %ebx
	movl %ebx, -8(%ebp)
	lea -8(%ebp), %ecx
	movl %ecx, -8(%ebp)
	movl -8(%ebp), %ecx
	movl %ecx, -8(%ebp)
	movl $0, %ecx
	movl %ecx, -4(%ebp)
	jmp .BB.1.2
.BB.1.2:
	movl -4(%ebp), %ecx
	movl $10, %ebx
	cmp %ebx, %ecx
	jl .BB.1.3
	jmp .BB.1.5
.BB.1.3:
	lea a, %ecx
	movl %ecx, -20(%ebp)
	movl -4(%ebp), %eax
	xor %edx, %edx
	movl $4, %ebx
	mul %ebx
	movl %eax, -24(%ebp)
	movl -20(%ebp), %ecx
	addl -24(%ebp), %ecx
	movl %ecx, -28(%ebp)
	movl -4(%ebp), %ecx
	movl -28(%ebp), %ebx
	movl %ecx, (%ebx)
	jmp .BB.1.4
.BB.1.4:
	movl -4(%ebp), %ecx
	addl $1, %ecx
	movl %ecx, -4(%ebp)
	jmp .BB.1.2
.BB.1.5:
	movl $0, %ecx
	movl %ecx, -4(%ebp)
	jmp .BB.1.6
.BB.1.6:
	movl -4(%ebp), %ecx
	movl $10, %ebx
	cmp %ebx, %ecx
	jl .BB.1.7
	jmp .BB.1.9
.BB.1.7:
	lea a, %ecx
	movl %ecx, -32(%ebp)
	movl -4(%ebp), %eax
	xor %edx, %edx
	movl $4, %ebx
	mul %ebx
	movl %eax, -36(%ebp)
	movl -32(%ebp), %ecx
	addl -36(%ebp), %ecx
	movl %ecx, -40(%ebp)
	movl -40(%ebp), %ecx
	movl (%ecx), %ebx
	movl %ebx, -44(%ebp)
	movl -44(%ebp), %ecx
	pushl %ecx
	movl -4(%ebp), %ecx
	pushl %ecx
	movl .string_0, %ecx
	pushl %ecx
	call printf
	addl $12, %esp
movl printf, %eax
	leave
	ret
	jmp .BB.1.8
.BB.1.8:
	movl -4(%ebp), %ecx
	addl $1, %ecx
	movl %ecx, -4(%ebp)
	jmp .BB.1.6
.BB.1.9:
	lea a, %ecx
	movl %ecx, -48(%ebp)
	movl $9, %eax
	xor %edx, %edx
	movl $4, %ebx
	mul %ebx
	movl %eax, -52(%ebp)
	movl -48(%ebp), %ecx
	addl -52(%ebp), %ecx
	movl %ecx, -56(%ebp)
	movl -56(%ebp), %ecx
	movl (%ecx), %ebx
	movl %ebx, -60(%ebp)
	movl -60(%ebp), %ecx
	movl $9, %ebx
	cmp %ebx, %ecx
	je .BB.1.10
	jmp .BB.1.11
.BB.1.10:
	movl .string_1, %ecx
	pushl %ecx
	call printf
	addl $4, %esp
movl printf, %eax
	leave
	ret
	jmp .BB.1.12
.BB.1.11:
	movl .string_2, %ecx
	pushl %ecx
	call printf
	addl $4, %esp
movl printf, %eax
	leave
	ret
	jmp .BB.1.12
.BB.1.12:
	movl -8(%ebp), %ecx
	movl (%ecx), %ebx
	movl %ebx, -64(%ebp)
	movl -64(%ebp), %ecx
	movl $4, %ebx
	cmp %ebx, %ecx
	je .BB.1.13
	jmp .BB.1.14
.BB.1.13:
	movl .string_3, %ecx
	pushl %ecx
	call printf
	addl $4, %esp
movl printf, %eax
	leave
	ret
	jmp .BB.1.15
.BB.1.14:
	movl .string_4, %ecx
	pushl %ecx
	call printf
	addl $4, %esp
movl printf, %eax
	leave
	ret
	jmp .BB.1.15
.BB.1.15:
	movl $69, %eax
	leave
	ret
	