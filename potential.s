hello_str: .asciz "Hello, world!\n"
.text
.global _start
_start:
movl $1, %eax
movl $1, %edi
lea hello_str, %rsi
movb $14, %dl
syscall
jmp EXIT
EXIT:
movq $60, %rax
movq $0, %rdi
syscall
