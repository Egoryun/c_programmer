.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $3, %eax
    pushq %rax
    movl $10, %eax
    popq %rbx
    subl %ebx, %eax
    popq %rbp
    ret
