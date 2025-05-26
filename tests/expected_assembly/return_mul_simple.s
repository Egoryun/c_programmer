.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $6, %eax
    pushq %rax
    movl $7, %eax
    popq %rbx
    imull %ebx, %eax
    popq %rbp
    ret
