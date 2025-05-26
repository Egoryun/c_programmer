.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    // Term (3 * 4)
    movl $4, %eax        // RHS of (3*4)
    pushq %rax
    movl $3, %eax        // LHS of (3*4)
    popq %rbx
    imull %ebx, %eax     // Result of (3*4) in %eax
    pushq %rax           // Push result of (3*4)
    // Expression 2 + (result of 3*4)
    movl $2, %eax        // LHS of 2 + ...
    popq %rbx
    addl %ebx, %eax      // Result of 2 + (3*4) in %eax
    popq %rbp
    ret
