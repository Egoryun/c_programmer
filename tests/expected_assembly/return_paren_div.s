.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    // For (10 / (1+1)):
    // RHS of '/' is (1+1). Evaluate (1+1):
    //   RHS of '+' is 1. Evaluate 1:
    movl $1, %eax
    pushq %rax
    //   LHS of '+' is 1. Evaluate 1:
    movl $1, %eax
    //   LHS (1) in %eax. Pop RHS (1) into %rbx:
    popq %rbx
    //   Add:
    addl %ebx, %eax // (1+1) result in %eax
    pushq %rax      // Push result of (1+1)
    // LHS of '/' is 10. Evaluate 10:
    movl $10, %eax
    // LHS (10) in %eax. Pop RHS (result of 1+1) into %rbx:
    popq %rbx
    // Divide:
    cdq
    idivl %ebx      // (10 / (1+1)) result in %eax
    popq %rbp
    ret
