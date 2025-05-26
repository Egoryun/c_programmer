.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    // For ((2+3)*4):
    // RHS of '*' is 4. Evaluate 4:
    movl $4, %eax
    pushq %rax
    // LHS of '*' is (2+3). Evaluate (2+3):
    //   RHS of '+' is 3. Evaluate 3:
    movl $3, %eax
    pushq %rax
    //   LHS of '+' is 2. Evaluate 2:
    movl $2, %eax
    //   LHS (2) in %eax. Pop RHS (3) into %rbx:
    popq %rbx
    //   Add:
    addl %ebx, %eax  // (2+3) result now in %eax
    // LHS of '*' (result of 2+3) is in %eax. Pop RHS of '*' (4) into %rbx:
    popq %rbx
    // Multiply:
    imull %ebx, %eax   // ((2+3)*4) result in %eax
    popq %rbp
    ret
