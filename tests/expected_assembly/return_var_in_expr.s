.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $4, %rsp     // Allocate for val (-4)
    movl $10, %eax    // RHS of val = 10
    movl %eax, -4(%rbp) // Assign to val
    // For val + 3:
    // RHS is 3
    movl $3, %eax
    pushq %rax
    // LHS is val
    movl -4(%rbp), %eax // Load val into %eax
    popq %rbx           // Pop 3 into %rbx
    addl %ebx, %eax     // val + 3
    movq %rbp, %rsp   // Deallocate locals
    popq %rbp
    ret
