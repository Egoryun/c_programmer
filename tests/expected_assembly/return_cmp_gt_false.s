.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $5, %eax
    pushq %rax
    movl $3, %eax
    popq %rbx
    cmpl %ebx, %eax
    setg %al
    movzbl %al, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
