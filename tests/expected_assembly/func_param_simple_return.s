.globl ret_param
ret_param:
    pushq %rbp
    movq %rsp, %rbp
    // No local vars, so no subq %rsp
    movl 8(%rbp), %eax  // Load p1 (first param) into %eax
    movq %rbp, %rsp
    popq %rbp
    ret

.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    // No local vars for main
    movl $42, %eax
    pushl %eax          // Push argument 42
    call ret_param
    addq $4, %rsp       // Clean up 1 argument
    // Result of ret_param is already in %eax for main's return
    movq %rbp, %rsp
    popq %rbp
    ret
