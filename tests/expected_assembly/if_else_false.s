.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $4, %rsp     // y
    movl $100, %eax
    movl %eax, -4(%rbp) // y = 100
    // Condition y < 10
    movl $10, %eax
    pushq %rax
    movl -4(%rbp), %eax // Load y
    popq %rbx
    cmpl %ebx, %eax
    setl %al
    movzbl %al, %eax
    cmpl $0, %eax
    je _Lelse0       // Jump to else (will jump)
    // Then block (skipped)
    movl $1, %eax
    pushq %rax
    movl -4(%rbp), %eax // Load y
    popq %rbx
    addl %ebx, %eax     // y + 1
    movl %eax, -4(%rbp) // y = y + 1
    jmp _Lendif1
_Lelse0:
    // Else block
    movl $2, %eax
    pushq %rax
    movl -4(%rbp), %eax // Load y
    popq %rbx
    imull %ebx, %eax    // y * 2
    movl %eax, -4(%rbp) // y = y * 2
_Lendif1:
    movl -4(%rbp), %eax // return y
    movq %rbp, %rsp
    popq %rbp
    ret
