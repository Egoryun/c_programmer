.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $3, %eax
    pushq %rax
    movl $7, %eax
    popq %rbx
    cdq
    idivl %ebx
    popq %rbp
    ret
