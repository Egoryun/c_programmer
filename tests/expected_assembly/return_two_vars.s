.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $8, %rsp     // Allocate for a (-4) and b (-8)
    movl $20, %eax    // RHS of a = 20
    movl %eax, -4(%rbp) // Assign to a
    movl $7, %eax     // RHS of b = 7
    movl %eax, -8(%rbp) // Assign to b
    // For a - b:
    // RHS is b
    movl -8(%rbp), %eax // Load b
    pushq %rax
    // LHS is a
    movl -4(%rbp), %eax // Load a
    popq %rbx           // Pop b into %rbx
    subl %ebx, %eax     // a - b
    movq %rbp, %rsp   // Deallocate locals
    popq %rbp
    ret
