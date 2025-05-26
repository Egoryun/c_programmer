.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $4, %rsp     // Allocate space for x (offset -4)
    movl $5, %eax     // RHS of x = 5
    movl %eax, -4(%rbp) // Assign to x
    movl -4(%rbp), %eax // Load x for return
    movq %rbp, %rsp   // Deallocate locals
    popq %rbp
    ret
