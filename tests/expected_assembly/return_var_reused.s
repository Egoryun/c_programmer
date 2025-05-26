.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $4, %rsp     // Allocate for x (-4)
    movl $1, %eax     // RHS of x = 1
    movl %eax, -4(%rbp) // Assign to x
    // For x = x + 9
    // RHS is x + 9
    //   RHS of + is 9
    movl $9, %eax
    pushq %rax
    //   LHS of + is x
    movl -4(%rbp), %eax // Load x
    popq %rbx           // Pop 9
    addl %ebx, %eax     // x + 9 result in %eax
    movl %eax, -4(%rbp) // Assign result back to x
    movl -4(%rbp), %eax // Load x for return
    movq %rbp, %rsp   // Deallocate locals
    popq %rbp
    ret
