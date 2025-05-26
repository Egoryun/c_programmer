.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    // Term (2 * 3)
    movl $3, %eax        // RHS of (2*3)
    pushq %rax
    movl $2, %eax        // LHS of (2*3)
    popq %rbx
    imull %ebx, %eax     // Result of (2*3) in %eax
    pushq %rax           // Push result of (2*3)
    // Expression 10 - (result of 2*3)
    movl $10, %eax       // LHS of 10 - ...
    popq %rbx
    subl %ebx, %eax      // Result of 10 - (2*3) in %eax
    popq %rbp
    ret
