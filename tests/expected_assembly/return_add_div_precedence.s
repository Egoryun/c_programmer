.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    // Term (10 / 2)
    movl $2, %eax        // RHS of (10/2)
    pushq %rax
    movl $10, %eax       // LHS of (10/2)
    popq %rbx
    cdq
    idivl %ebx           // Result of (10/2) in %eax
    pushq %rax           // Push result of (10/2)
    // Expression 5 + (result of 10/2)
    movl $5, %eax        // LHS of 5 + ...
    popq %rbx
    addl %ebx, %eax      // Result of 5 + (10/2) in %eax
    popq %rbp
    ret
