.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    // For (((1+1)*2)+3):
    // RHS of outer '+' is 3. Evaluate 3:
    movl $3, %eax
    pushq %rax
    // LHS of outer '+' is ((1+1)*2). Evaluate ((1+1)*2):
    //   RHS of '*' is 2. Evaluate 2:
    movl $2, %eax
    pushq %rax
    //   LHS of '*' is (1+1). Evaluate (1+1):
    //     RHS of inner '+' is 1. Evaluate 1:
    movl $1, %eax
    pushq %rax
    //     LHS of inner '+' is 1. Evaluate 1:
    movl $1, %eax
    //     LHS (1) in %eax. Pop RHS (1) into %rbx:
    popq %rbx
    //     Add:
    addl %ebx, %eax  // (1+1) result in %eax
    //   LHS of '*' (result of 1+1) is in %eax. Pop RHS of '*' (2) into %rbx:
    popq %rbx
    //   Multiply:
    imull %ebx, %eax // ((1+1)*2) result in %eax
    // LHS of outer '+' (result of ((1+1)*2)) is in %eax. Pop RHS of outer '+' (3) into %rbx:
    popq %rbx
    // Add:
    addl %ebx, %eax  // (((1+1)*2)+3) result in %eax
    popq %rbp
    ret
