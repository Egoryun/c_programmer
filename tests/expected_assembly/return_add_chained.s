.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $3, %eax        # RHS of (2+3)
    pushq %rax
    movl $2, %eax        # LHS of (2+3)
    popq %rbx
    addl %ebx, %eax      # Result of (2+3) in %eax
    pushq %rax           # Push result of (2+3)
    movl $1, %eax        # LHS of 1 + (2+3)
    popq %rbx
    addl %ebx, %eax      # Result of 1 + (2+3) in %eax
    popq %rbp
    ret
