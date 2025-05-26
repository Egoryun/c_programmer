.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $4, %rsp     // Space for x
    movl $0, %eax
    movl %eax, -4(%rbp) // x = 0
    // Condition 1 == 0
    movl $0, %eax
    pushq %rax
    movl $1, %eax
    popq %rbx
    cmpl %ebx, %eax
    sete %al
    movzbl %al, %eax  // Result of 1 == 0 in %eax
    cmpl $0, %eax
    je _Lendif0      // Jump if false (will jump)
    // Then block (skipped)
    movl $10, %eax
    movl %eax, -4(%rbp) // x = 10
_Lendif0:
    movl -4(%rbp), %eax // return x
    movq %rbp, %rsp
    popq %rbp
    ret
