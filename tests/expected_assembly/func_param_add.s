.globl add_params
add_params:
    pushq %rbp
    movq %rsp, %rbp
    // No local vars for add_params
    // For a + b:
    // RHS is b (second param, 12(%rbp))
    movl 12(%rbp), %eax
    pushq %rax
    // LHS is a (first param, 8(%rbp))
    movl 8(%rbp), %eax
    popq %rbx
    addl %ebx, %eax   // a + b
    movq %rbp, %rsp
    popq %rbp
    ret

.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    // No local vars for main
    // Args for add_params(10, 5) - push right to left
    movl $5, %eax       // Arg b
    pushl %eax
    movl $10, %eax      // Arg a
    pushl %eax
    call add_params
    addq $8, %rsp       // Clean up 2 arguments (4*2=8)
    // Result of add_params is in %eax
    movq %rbp, %rsp
    popq %rbp
    ret
