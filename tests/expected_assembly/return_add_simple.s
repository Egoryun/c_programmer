.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $3, %eax
    pushq %rax
    movl $5, %eax
    popq %rbx
    addl %ebx, %eax
    popq %rbp
    ret
