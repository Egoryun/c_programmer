.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $2, %eax
    pushq %rax
    movl $10, %eax
    popq %rbx
    cdq
    idivl %ebx
    popq %rbp
    ret
