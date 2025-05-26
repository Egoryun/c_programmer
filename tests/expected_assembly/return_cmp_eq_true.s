.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $5, %eax      // RHS of ==
    pushq %rax
    movl $5, %eax      // LHS of ==
    popq %rbx
    cmpl %ebx, %eax
    sete %al
    movzbl %al, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
