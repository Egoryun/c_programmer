.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    // For ((10-2)*3):
    // RHS of '*' is 3. Evaluate 3:
    movl $3, %eax
    pushq %rax
    // LHS of '*' is (10-2). Evaluate (10-2):
    //   RHS of '-' is 2. Evaluate 2:
    movl $2, %eax
    pushq %rax
    //   LHS of '-' is 10. Evaluate 10:
    movl $10, %eax
    //   LHS (10) in %eax. Pop RHS (2) into %rbx:
    popq %rbx
    //   Subtract:
    subl %ebx, %eax  // (10-2) result now in %eax
    // LHS of '*' (result of 10-2) is in %eax. Pop RHS of '*' (3) into %rbx:
    popq %rbx
    // Multiply:
    imull %ebx, %eax   // ((10-2)*3) result in %eax
    popq %rbp
    ret
