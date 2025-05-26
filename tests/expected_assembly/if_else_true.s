.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $4, %rsp     // Space for x
    // Condition 5 > 3
    movl $3, %eax
    pushq %rax
    movl $5, %eax
    popq %rbx
    cmpl %ebx, %eax
    setg %al
    movzbl %al, %eax  // Result of 5 > 3 in %eax
    cmpl $0, %eax
    je _Lelse0       // Jump to else if false (will not jump)
    // Then block
    movl $1, %eax
    movl %eax, -4(%rbp) // x = 1
    jmp _Lendif1     // Skip else block
_Lelse0:
    // Else block (skipped)
    movl $2, %eax
    movl %eax, -4(%rbp) // x = 2
_Lendif1:
    movl -4(%rbp), %eax // return x
    movq %rbp, %rsp
    popq %rbp
    ret
