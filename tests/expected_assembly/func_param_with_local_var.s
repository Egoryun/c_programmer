.globl calc
calc:
    pushq %rbp
    movq %rsp, %rbp
    subq $4, %rsp     // Space for local var lv (-4(%rbp))
    // lv = p * 2:
    // RHS is p * 2
    //   RHS of * is 2
    movl $2, %eax
    pushq %rax
    //   LHS of * is p (param, 8(%rbp))
    movl 8(%rbp), %eax  // Load p
    popq %rbx
    imull %ebx, %eax    // p * 2 result in %eax
    movl %eax, -4(%rbp) // Assign to lv
    // return lv + 3:
    // RHS is 3
    movl $3, %eax
    pushq %rax
    // LHS is lv (local, -4(%rbp))
    movl -4(%rbp), %eax // Load lv
    popq %rbx
    addl %ebx, %eax     // lv + 3
    movq %rbp, %rsp
    popq %rbp
    ret

.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    // No local vars for main
    movl $7, %eax
    pushl %eax          // Arg for calc
    call calc
    addq $4, %rsp       // Clean up 1 argument
    // Result of calc is in %eax
    movq %rbp, %rsp
    popq %rbp
    ret
