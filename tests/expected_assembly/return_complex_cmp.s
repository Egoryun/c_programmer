.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    // RHS of == is (10 - 5). Evaluate (10 - 5):
    //   RHS of '-' is 5. Evaluate 5:
    movl $5, %eax
    pushq %rax
    //   LHS of '-' is 10. Evaluate 10:
    movl $10, %eax
    //   LHS (10) in %eax. Pop RHS (5) into %rbx:
    popq %rbx
    //   Subtract:
    subl %ebx, %eax  // (10-5) result in %eax
    pushq %rax       // Push result of (10-5)
    // LHS of == is (1 + 4). Evaluate (1 + 4):
    //   RHS of '+' is 4. Evaluate 4:
    movl $4, %eax
    pushq %rax
    //   LHS of '+' is 1. Evaluate 1:
    movl $1, %eax
    //   LHS (1) in %eax. Pop RHS (4) into %rbx:
    popq %rbx
    //   Add:
    addl %ebx, %eax  // (1+4) result in %eax
    // LHS (result of 1+4) in %eax. Pop RHS (result of 10-5) into %rbx:
    popq %rbx
    // Compare:
    cmpl %ebx, %eax
    sete %al
    movzbl %al, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
