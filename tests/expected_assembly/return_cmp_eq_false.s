.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $3, %eax
    pushq %rax
    movl $5, %eax
    popq %rbx
    cmpl %ebx, %eax
    sete %al
    movzbl %al, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
