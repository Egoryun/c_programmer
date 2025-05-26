.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $5, %eax
    pushq %rax
    movl $5, %eax
    popq %rbx
    cmpl %ebx, %eax
    setle %al
    movzbl %al, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
